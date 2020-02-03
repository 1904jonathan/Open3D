// ----------------------------------------------------------------------------
// -                        Open3D: www.open3d.org                            -
// ----------------------------------------------------------------------------
// The MIT License (MIT)
//
// Copyright (c) 2020 www.open3d.org
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
// ----------------------------------------------------------------------------

#pragma once
#define EIGEN_USE_GPU
#include "Open3D/ML/Misc/Detail/FixedRadiusSearch.h"
#include "Open3D/ML/Misc/Detail/MemoryAllocation.h"
#include "Open3D/Utility/Helper.h"

#include "cub/cub.cuh"

using namespace open3d::utility;

#define DEBUG_FLAG false

namespace open3d {
namespace ml {
namespace detail {

namespace {

template <class T>
using Vec3 = Eigen::Matrix<T, 3, 1>;

/// Computes the distance of two points and tests if the distance is below a
/// threshold.
///
/// \tparam METRIC    The distance metric. One of L1, L2, Linf.
/// \tparam T    Floating point type for the distances.
///
/// \param p1           A 3D point
/// \param p2           Another 3D point
/// \param dist         Output parameter for the distance.
/// \param threshold    The scalar threshold.
///
/// \return Returns true if the distance is <= threshold.
///
template <int METRIC = L2, class T>
inline __device__ bool NeighborTest(const Vec3<T>& p1,
                                    const Vec3<T>& p2,
                                    T* dist,
                                    T threshold) {
    bool result = false;
    if (METRIC == Linf) {
        Vec3<T> d = (p1 - p2).cwiseAbs();
        *dist = d[0] > d[1] ? d[0] : d[1];
        *dist = *dist > d[2] ? *dist : d[2];
    } else if (METRIC == L1) {
        Vec3<T> d = (p1 - p2).cwiseAbs();
        *dist = (d[0] + d[1] + d[2]);
    } else {
        Vec3<T> d = p1 - p2;
        *dist = d.dot(d);
    }
    result = *dist <= threshold;
    return result;
}

/// Computes an integer voxel index for a 3D position.
///
/// \param pos               A 3D position.
/// \param inv_voxel_size    The reciprocal of the voxel size
///
template <class T>
inline __device__ Vec3<int> ComputeVoxelIndex(Vec3<T> pos, T inv_voxel_size) {
    Vec3<T> coord = inv_voxel_size * pos;
    return {int(floor(coord[0])), int(floor(coord[1])), int(floor(coord[2]))};
}

/// Spatial hashing function for integer coordinates.
inline __device__ size_t SpatialHash(int x, int y, int z) {
    return x * 73856096 ^ y * 193649663 ^ z * 83492791;
}

/// Kernel for CountHashTableEntries
template <class T>
__global__ void CountHashTableEntriesKernel(uint32_t* count_table,
                                            size_t count_table_size,
                                            T inv_voxel_size,
                                            const T* const __restrict__ points,
                                            size_t num_points) {
    const int idx = blockDim.x * blockIdx.x + threadIdx.x;
    if (idx >= num_points) return;

    Vec3<T> pos(
            {points[idx * 3 + 0], points[idx * 3 + 1], points[idx * 3 + 2]});

    Vec3<int> voxel_index = ComputeVoxelIndex(pos, inv_voxel_size);
    size_t hash = SpatialHash(voxel_index[0], voxel_index[1], voxel_index[2]) %
                  count_table_size;
    atomicAdd(&count_table[hash], 1);
}

/// Counts for each hash entry the number of points that map to this entry.
///
/// \param count_table    Pointer to the table for counting.
///
/// \param count_table_size    This is the size of the hash table.
///
/// \param inv_voxel_size    Reciproval of the voxel size
///
/// \param points    Array with the 3D point positions.
///
/// \param num_points    The number of points.
///
template <class T>
void CountHashTableEntries(const cudaStream_t& stream,
                           uint32_t* count_table,
                           size_t count_table_size,
                           T inv_voxel_size,
                           const T* points,
                           size_t num_points) {
    cudaMemsetAsync(count_table, 0, sizeof(uint32_t) * count_table_size,
                    stream);

    const int BLOCKSIZE = 64;
    dim3 block(BLOCKSIZE, 1, 1);
    dim3 grid(0, 1, 1);
    grid.x = DivUp(num_points, block.x);

    if (grid.x)
        CountHashTableEntriesKernel<T><<<grid, block, 0, stream>>>(
                count_table, count_table_size, inv_voxel_size, points,
                num_points);
}

/// Kernel for ComputePointIndexTable
template <class T>
__global__ void ComputePointIndexTableKernel(
        uint32_t* __restrict__ point_index_table,
        uint32_t* __restrict__ count_tmp,
        const uint32_t* const __restrict__ hash_table_prefix_sum,
        size_t hash_table_size,
        T inv_voxel_size,
        const T* const __restrict__ points,
        size_t num_points) {
    const int idx = blockDim.x * blockIdx.x + threadIdx.x;
    if (idx >= num_points) return;

    Vec3<T> pos(
            {points[idx * 3 + 0], points[idx * 3 + 1], points[idx * 3 + 2]});

    Vec3<int> voxel_index = ComputeVoxelIndex(pos, inv_voxel_size);
    size_t hash = SpatialHash(voxel_index[0], voxel_index[1], voxel_index[2]) %
                  hash_table_size;

    point_index_table[hash_table_prefix_sum[hash] +
                      atomicAdd(&count_tmp[hash], 1)] = idx;
}

/// Writes the index of the points to the hash cells.
///
/// \param point_index_table    The output array storing the point indices for
/// all
///        cells. Start and end of each cell is defined by \p
///        hash_table_prefix_sum
///
/// \param count_tmp    Temporary memory of size \p hash_table_size .
///
/// \param hash_table_prefix_sum    The prefix sum describing the start and end
///        of each cell.
///
/// \param hash_table_size    The size of the hash table.
///
/// \param inv_voxel_size    Reciproval of the voxel size
///
/// \param points    Array with the 3D point positions.
///
/// \param num_points    The number of points.
///
template <class T>
void ComputePointIndexTable(
        const cudaStream_t& stream,
        uint32_t* __restrict__ point_index_table,
        uint32_t* __restrict__ count_tmp,
        const uint32_t* const __restrict__ hash_table_prefix_sum,
        size_t hash_table_size,
        T inv_voxel_size,
        const T* const __restrict__ points,
        size_t num_points) {
    cudaMemsetAsync(count_tmp, 0, sizeof(uint32_t) * hash_table_size, stream);

    const int BLOCKSIZE = 64;
    dim3 block(BLOCKSIZE, 1, 1);
    dim3 grid(0, 1, 1);
    grid.x = DivUp(num_points, block.x);

    if (grid.x)
        ComputePointIndexTableKernel<T><<<grid, block, 0, stream>>>(
                point_index_table, count_tmp, hash_table_prefix_sum,
                hash_table_size, inv_voxel_size, points, num_points);
}

/// Kernel for CountNeighbors
template <int METRIC, bool IGNORE_QUERY_POINT, class T>
__global__ void CountNeighborsKernel(
        uint32_t* __restrict__ neighbors_count,
        const uint32_t* const __restrict__ point_index_table,
        const uint32_t* const __restrict__ hash_table_prefix_sum,
        size_t hash_table_size,
        const T* const __restrict__ query_points,
        size_t num_queries,
        const T* const __restrict__ points,
        size_t num_points,
        const T inv_voxel_size,
        const T radius,
        const T threshold) {
    int query_idx = blockDim.x * blockIdx.x + threadIdx.x;
    if (query_idx >= num_queries) return;

    int count = 0;  // counts the number of neighbors for this query point

    Vec3<T> query_pos({query_points[query_idx * 3 + 0],
                       query_points[query_idx * 3 + 1],
                       query_points[query_idx * 3 + 2]});
    Vec3<int> voxel_index = ComputeVoxelIndex(query_pos, inv_voxel_size);
    int hash = SpatialHash(voxel_index[0], voxel_index[1], voxel_index[2]) %
               hash_table_size;

    int bins_to_visit[8] = {hash, -1, -1, -1, -1, -1, -1, -1};

    for (int dz = -1; dz <= 1; dz += 2)
        for (int dy = -1; dy <= 1; dy += 2)
            for (int dx = -1; dx <= 1; dx += 2) {
                Vec3<T> p = query_pos + radius * Vec3<T>({T(dx), T(dy), T(dz)});
                voxel_index = ComputeVoxelIndex(p, inv_voxel_size);
                hash = SpatialHash(voxel_index[0], voxel_index[1],
                                   voxel_index[2]) %
                       hash_table_size;

                // insert without duplicates
                for (int i = 0; i < 8; ++i) {
                    if (bins_to_visit[i] == hash) {
                        break;
                    } else if (bins_to_visit[i] == -1) {
                        bins_to_visit[i] = hash;
                        break;
                    }
                }
            }

    for (int bin_i = 0; bin_i < 8; ++bin_i) {
        int bin = bins_to_visit[bin_i];
        if (bin == -1) break;

        size_t begin_idx = hash_table_prefix_sum[bin];
        size_t end_idx =
                (bin + 1 < hash_table_size ? hash_table_prefix_sum[bin + 1]
                                           : num_points);

        for (size_t j = begin_idx; j < end_idx; ++j) {
            uint32_t idx = point_index_table[j];

            Vec3<T> p({points[idx * 3 + 0], points[idx * 3 + 1],
                       points[idx * 3 + 2]});
            if (IGNORE_QUERY_POINT) {
                if (query_pos == p) continue;
            }

            T dist;
            if (NeighborTest<METRIC>(p, query_pos, &dist, threshold)) ++count;
        }
    }
    neighbors_count[query_idx] = count;
}

/// Count the number of neighbors for each query point
///
/// \param neighbors_count    Output array for counting the number of neighbors.
///        The size of the array is \p num_queries.
///
/// \param point_index_table    The array storing the point indices for all
///        cells. Start and end of each cell is defined by \p
///        hash_table_prefix_sum
///
/// \param hash_table_prefix_sum    The prefix sum describing the start and end
///        of each cell.
///
/// \param hash_table_size    The size of the hash table.
///
/// \param query_points    Array with the 3D query positions. This may be the
///        same array as \p points.
///
/// \param num_queries    The number of query points.
///
/// \param points    Array with the 3D point positions.
///
/// \param num_points    The number of points.
///
/// \param inv_voxel_size    Reciproval of the voxel size
///
/// \param radius    The search radius.
///
/// \param metric    One of L1, L2, Linf. Defines the distance metric for the
///        search.
///
/// \param ignore_query_point    If true then points with the same position as
///        the query point will be ignored.
///
template <class T>
void CountNeighbors(const cudaStream_t& stream,
                    uint32_t* neighbors_count,
                    const uint32_t* const point_index_table,
                    const uint32_t* const hash_table_prefix_sum,
                    size_t hash_table_size,
                    const T* const query_points,
                    size_t num_queries,
                    const T* const points,
                    size_t num_points,
                    const T inv_voxel_size,
                    const T radius,
                    const Metric metric,
                    const bool ignore_query_point) {
    const T threshold = (metric == L2 ? radius * radius : radius);

    const int BLOCKSIZE = 64;
    dim3 block(BLOCKSIZE, 1, 1);
    dim3 grid(0, 1, 1);
    grid.x = DivUp(num_queries, block.x);

    if (grid.x) {
#define FN_PARAMETERS                                                       \
    neighbors_count, point_index_table, hash_table_prefix_sum,              \
            hash_table_size, query_points, num_queries, points, num_points, \
            inv_voxel_size, radius, threshold

#define CALL_TEMPLATE(METRIC)                                    \
    if (METRIC == metric) {                                      \
        if (ignore_query_point)                                  \
            CountNeighborsKernel<METRIC, true, T>                \
                    <<<grid, block, 0, stream>>>(FN_PARAMETERS); \
        else                                                     \
            CountNeighborsKernel<METRIC, false, T>               \
                    <<<grid, block, 0, stream>>>(FN_PARAMETERS); \
    }

        CALL_TEMPLATE(L1)
        CALL_TEMPLATE(L2)
        CALL_TEMPLATE(Linf)

#undef CALL_TEMPLATE
#undef FN_PARAMETERS
    }
}

/// Kernel for WriteNeighborsIndicesAndDistances
template <class T, int METRIC, bool IGNORE_QUERY_POINT, bool RETURN_DISTANCES>
__global__ void WriteNeighborsIndicesAndDistancesKernel(
        int32_t* __restrict__ indices,
        T* __restrict__ distances,
        const int64_t* const __restrict__ neighbors_row_splits,
        const uint32_t* const __restrict__ point_index_table,
        const uint32_t* const __restrict__ hash_table_prefix_sum,
        size_t hash_table_size,
        const T* const __restrict__ query_points,
        size_t num_queries,
        const T* const __restrict__ points,
        size_t num_points,
        const T inv_voxel_size,
        const T radius,
        const T threshold) {
    int query_idx = blockDim.x * blockIdx.x + threadIdx.x;
    if (query_idx >= num_queries) return;

    int count = 0;  // counts the number of neighbors for this query point

    size_t indices_offset = neighbors_row_splits[query_idx];

    Vec3<T> query_pos({query_points[query_idx * 3 + 0],
                       query_points[query_idx * 3 + 1],
                       query_points[query_idx * 3 + 2]});
    Vec3<int> voxel_index = ComputeVoxelIndex(query_pos, inv_voxel_size);
    int hash = SpatialHash(voxel_index[0], voxel_index[1], voxel_index[2]) %
               hash_table_size;

    int bins_to_visit[8] = {hash, -1, -1, -1, -1, -1, -1, -1};

    for (int dz = -1; dz <= 1; dz += 2)
        for (int dy = -1; dy <= 1; dy += 2)
            for (int dx = -1; dx <= 1; dx += 2) {
                Vec3<T> p = query_pos + radius * Vec3<T>({T(dx), T(dy), T(dz)});
                voxel_index = ComputeVoxelIndex(p, inv_voxel_size);
                hash = SpatialHash(voxel_index[0], voxel_index[1],
                                   voxel_index[2]) %
                       hash_table_size;

                // insert without duplicates
                for (int i = 0; i < 8; ++i) {
                    if (bins_to_visit[i] == hash) {
                        break;
                    } else if (bins_to_visit[i] == -1) {
                        bins_to_visit[i] = hash;
                        break;
                    }
                }
            }

    for (int bin_i = 0; bin_i < 8; ++bin_i) {
        int bin = bins_to_visit[bin_i];
        if (bin == -1) break;

        size_t begin_idx = hash_table_prefix_sum[bin];
        size_t end_idx =
                (bin + 1 < hash_table_size ? hash_table_prefix_sum[bin + 1]
                                           : num_points);

        for (size_t j = begin_idx; j < end_idx; ++j) {
            uint32_t idx = point_index_table[j];

            Vec3<T> p({points[idx * 3 + 0], points[idx * 3 + 1],
                       points[idx * 3 + 2]});
            if (IGNORE_QUERY_POINT) {
                if (query_pos == p) continue;
            }

            T dist;
            if (NeighborTest<METRIC>(p, query_pos, &dist, threshold)) {
                indices[indices_offset + count] = idx;
                if (RETURN_DISTANCES) {
                    distances[indices_offset + count] = dist;
                }
                ++count;
            }
        }
    }
}

/// Count the number of neighbors for each query point
///
/// \param indices    Output array with the neighbors indices.
///
/// \param distances    Output array with the neighbors distances. May be null
///        if return_distances is false.
///
/// \param neighbors_row_splits    This is the prefix sum which describes
///        start and end of the neighbors and distances for each query point.
///
/// \param point_index_table    The array storing the point indices for all
///        cells. Start and end of each cell is defined by \p
///        hash_table_prefix_sum
///
/// \param hash_table_prefix_sum    The prefix sum describing the start and end
///        of each cell.
///
/// \param hash_table_size    The size of the hash table.
///
/// \param query_points    Array with the 3D query positions. This may be the
///        same array as \p points.
///
/// \param num_queries    The number of query points.
///
/// \param points    Array with the 3D point positions.
///
/// \param num_points    The number of points.
///
/// \param inv_voxel_size    Reciproval of the voxel size
///
/// \param radius    The search radius.
///
/// \param metric    One of L1, L2, Linf. Defines the distance metric for the
///        search.
///
/// \param ignore_query_point    If true then points with the same position as
///        the query point will be ignored.
///
/// \param return_distances    If true then this function will return the
///        distances for each neighbor to its query point in the same format
///        as the indices.
///        Note that for the L2 metric the squared distances will be returned!!
template <class T>
void WriteNeighborsIndicesAndDistances(
        const cudaStream_t& stream,
        int32_t* indices,
        T* distances,
        const int64_t* const neighbors_row_splits,
        const uint32_t* const point_index_table,
        const uint32_t* const hash_table_prefix_sum,
        size_t hash_table_size,
        const T* const query_points,
        size_t num_queries,
        const T* const points,
        size_t num_points,
        const T inv_voxel_size,
        const T radius,
        const Metric metric,
        const bool ignore_query_point,
        const bool return_distances) {
    const T threshold = (metric == L2 ? radius * radius : radius);

    const int BLOCKSIZE = 64;
    dim3 block(BLOCKSIZE, 1, 1);
    dim3 grid(0, 1, 1);
    grid.x = DivUp(num_queries, block.x);

    if (grid.x) {
#define FN_PARAMETERS                                                          \
    indices, distances, neighbors_row_splits, point_index_table,               \
            hash_table_prefix_sum, hash_table_size, query_points, num_queries, \
            points, num_points, inv_voxel_size, radius, threshold

#define CALL_TEMPLATE(METRIC, IGNORE_QUERY_POINT, RETURN_DISTANCES)            \
    if (METRIC == metric && IGNORE_QUERY_POINT == ignore_query_point &&        \
        RETURN_DISTANCES == return_distances) {                                \
        WriteNeighborsIndicesAndDistancesKernel<T, METRIC, IGNORE_QUERY_POINT, \
                                                RETURN_DISTANCES>              \
                <<<grid, block, 0, stream>>>(FN_PARAMETERS);                   \
    }

#define CALL_TEMPLATE2(METRIC)         \
    CALL_TEMPLATE(METRIC, true, true)  \
    CALL_TEMPLATE(METRIC, true, false) \
    CALL_TEMPLATE(METRIC, false, true) \
    CALL_TEMPLATE(METRIC, false, false)

#define CALL_TEMPLATE3 \
    CALL_TEMPLATE2(L1) \
    CALL_TEMPLATE2(L2) \
    CALL_TEMPLATE2(Linf)

        CALL_TEMPLATE3

#undef CALL_TEMPLATE
#undef CALL_TEMPLATE2
#undef CALL_TEMPLATE3
#undef FN_PARAMETERS
    }
}

}  // namespace

/// Fixed radius search. This function computes a list of neighbor indices
/// for each query point. The lists are stored linearly and an exclusive prefix
/// sum defines the start and end of list in the array.
/// In addition the function optionally can return the distances for each
/// neighbor in the same format as the indices to the neighbors.
///
/// All pointer arguments point to device memory unless stated otherwise.
///
/// \tparam T    Floating-point data type for the point positions.
///
/// \tparam OUTPUT_ALLOCATOR    Type of the output_allocator. See
///         \p output_allocator for more information.
///
///
/// \param temp    Pointer to temporary memory. If nullptr then the required
///        size of temporary memory will be written to \p temp_size and no
///        work is done.
///
/// \param temp_size    The size of the temporary memory in bytes. This is
///        used as an output if temp is nullptr
///
/// \param texture_alignment    The texture alignment in bytes. This is used
///        for allocating segments within the temporary memory.
///
/// \param query_neighbors_row_splits    This is the output pointer for the
///        prefix sum. The length of this array is \p num_queries + 1.
///
/// \param num_points    The number of points.
///
/// \param points    Array with the 3D point positions. This may be the same
///        array as \p queries.
///
/// \param num_queries    The number of query points.
///
/// \param queries    Array with the 3D query positions. This may be the same
///                   array as \p points.
///
/// \param radius    The search radius.
///
/// \param hash_table_size    The size of the hash table as number of entries.
///        This should be smaller than \p num_points.
///
/// \param metric    One of L1, L2, Linf. Defines the distance metric for the
///        search.
///
/// \param ignore_query_point    If true then points with the same position as
///        the query point will be ignored.
///
/// \param return_distances    If true then this function will return the
///        distances for each neighbor to its query point in the same format
///        as the indices.
///        Note that for the L2 metric the squared distances will be returned!!
///
/// \param output_allocator    An object that implements functions for
///         allocating the output arrays. The object must implement functions
///         AllocIndices(int32_t** ptr, size_t size) and
///         AllocDistances(T** ptr, size_t size). Both functions should
///         allocate memory and return a pointer to that memory in ptr.
///         Argument size specifies the size of the array as the number of
///         elements. Both functions must accept the argument size==0.
///         In this case ptr does not need to be set.
///
template <class T, class OUTPUT_ALLOCATOR>
void FixedRadiusSearchCUDA(const cudaStream_t& stream,
                           void* temp,
                           size_t& temp_size,
                           int texture_alignment,
                           int64_t* query_neighbors_row_splits,
                           size_t num_points,
                           const T* const points,
                           size_t num_queries,
                           const T* const queries,
                           const T radius,
                           size_t hash_table_size,
                           const Metric metric,
                           const bool ignore_query_point,
                           const bool return_distances,
                           OUTPUT_ALLOCATOR& output_allocator) {
    const bool get_temp_size = !temp;

    if (get_temp_size) {
        temp = (char*)1;  // worst case pointer alignment
        temp_size = std::numeric_limits<int64_t>::max();
    }

    // return empty output arrays if there are no points
    if ((0 == num_points || 0 == num_queries) && !get_temp_size) {
        cudaMemsetAsync(query_neighbors_row_splits, 0,
                        sizeof(int64_t) * (num_queries + 1), stream);
        int32_t* indices_ptr;
        output_allocator.AllocIndices(&indices_ptr, 0);

        T* distances_ptr;
        output_allocator.AllocDistances(&distances_ptr, 0);

        return;
    }

    MemoryAllocation mem_temp(temp, temp_size, texture_alignment);

    std::pair<uint32_t*, size_t> index_table =
            mem_temp.Alloc<uint32_t>(num_points);
    std::pair<uint32_t*, size_t> count_prefix_sum =
            mem_temp.Alloc<uint32_t>(hash_table_size);
    std::pair<uint32_t*, size_t> count_tmp =
            mem_temp.Alloc<uint32_t>(hash_table_size);

    const T voxel_size = 2 * radius;
    const T inv_voxel_size = 1 / voxel_size;

    // count number of points per hash entry
    if (!get_temp_size) {
        CountHashTableEntries(stream, count_tmp.first, count_tmp.second,
                              inv_voxel_size, points, num_points);
    }

    // compute prefix sum of the hash entry counts and store in count
    {
        std::pair<void*, size_t> exclusive_scan_temp(nullptr, 0);
        cub::DeviceScan::ExclusiveSum(exclusive_scan_temp.first,
                                      exclusive_scan_temp.second,
                                      count_tmp.first, count_prefix_sum.first,
                                      count_tmp.second, stream);

        exclusive_scan_temp = mem_temp.Alloc(exclusive_scan_temp.second);

        if (!get_temp_size) {
            cub::DeviceScan::ExclusiveSum(
                    exclusive_scan_temp.first, exclusive_scan_temp.second,
                    count_tmp.first, count_prefix_sum.first, count_tmp.second,
                    stream);
        }

        mem_temp.Free(exclusive_scan_temp);
    }

    // now compute the global indices which allows us to lookup the point index
    // for the entries in the hash cell
    if (!get_temp_size) {
        ComputePointIndexTable(stream, index_table.first, count_tmp.first,
                               count_prefix_sum.first, count_prefix_sum.second,
                               inv_voxel_size, points, num_points);
    }

    mem_temp.Free(count_tmp);

    std::pair<uint32_t*, size_t> query_neighbors_count =
            mem_temp.Alloc<uint32_t>(num_queries);

    // we need this value to compute the size of the index array
    if (!get_temp_size) {
        CountNeighbors(stream, query_neighbors_count.first, index_table.first,
                       count_prefix_sum.first, hash_table_size, queries,
                       num_queries, points, num_points, inv_voxel_size, radius,
                       metric, ignore_query_point);
    }

    // we need this value to compute the size of the index array
    int64_t last_prefix_sum_entry = 0;
    {
        std::pair<void*, size_t> inclusive_scan_temp(nullptr, 0);
        cub::DeviceScan::InclusiveSum(
                inclusive_scan_temp.first, inclusive_scan_temp.second,
                query_neighbors_count.first, query_neighbors_row_splits + 1,
                num_queries, stream);

        inclusive_scan_temp = mem_temp.Alloc(inclusive_scan_temp.second);

        if (!get_temp_size) {
            // set first element to zero
            cudaMemsetAsync(query_neighbors_row_splits, 0, sizeof(int64_t),
                            stream);
            cub::DeviceScan::InclusiveSum(
                    inclusive_scan_temp.first, inclusive_scan_temp.second,
                    query_neighbors_count.first, query_neighbors_row_splits + 1,
                    num_queries, stream);

            // get the last value
            cudaMemcpyAsync(&last_prefix_sum_entry,
                            query_neighbors_row_splits + num_queries,
                            sizeof(int64_t), cudaMemcpyDeviceToHost, stream);
            // wait for the async copies
            while (cudaErrorNotReady == cudaStreamQuery(stream)) { /*empty*/
            }
        }
        mem_temp.Free(inclusive_scan_temp);
    }

    mem_temp.Free(query_neighbors_count);

    if (get_temp_size) {
        // return the memory peak as the required temporary memory size.
        temp_size = mem_temp.MaxUsed();
        return;
    }

    // allocate the output array for the neighbor indices
    const size_t num_indices = last_prefix_sum_entry;
    int32_t* indices_ptr;
    output_allocator.AllocIndices(&indices_ptr, num_indices);

    T* distances_ptr;
    if (return_distances)
        output_allocator.AllocDistances(&distances_ptr, num_indices);
    else
        output_allocator.AllocDistances(&distances_ptr, 0);

    if (!get_temp_size) {
        WriteNeighborsIndicesAndDistances(
                stream, indices_ptr, distances_ptr, query_neighbors_row_splits,
                index_table.first, count_prefix_sum.first, hash_table_size,
                queries, num_queries, points, num_points, inv_voxel_size,
                radius, metric, ignore_query_point, return_distances);
    }
}

}  // namespace detail
}  // namespace ml
}  // namespace open3d
