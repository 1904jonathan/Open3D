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
#include <vector>

namespace open3d {
namespace ml {
namespace impl {

/// Copies and transforms the features to a column, which can be multiplied
/// with the filter matrix.
///
/// \tparam TReal    Type for positions and features.
/// \tparam TIndex    Type for addressing neighbors.
///
/// \param columns    Output array with shape
///        [num_out, spatial filter dims, in_channels].
///
/// \param in_channels    Number of input channels.
///
/// \param begin_idx    Index of the first output point to process.
///
/// \param end_idx    Index after the last output point to process.
///
/// \param num_out    The number of output points.
///
/// \param num_inp    The number of input points.
///
/// \param inp_features    The input features with shape
///        [num_inp, in_channels].
///
/// \param inp_importance    Optional importance for each input point with
///        shape [num_inp]. Set to null to disable.
///
/// \param neighbors_index_size    The size of the neighbors_index array.
///
/// \param neighbors_index    The array with lists of neighbors for each output
///        point. The start and end of each sublist is defined by
///        \p neighbors_row_splits.
///
/// \param neighbors_kernel_index    Defines which kernel element to use for
///        each neighbor. This array has the same length as \p neighbors_index.
///
/// \param neighbors_importance    Optional importance for each entry in
///        \p neighbors_index. Set to null to disable.
///
/// \param neighbors_row_splits   The prefix sum which defines the start
///        and end of the sublists in \p neighbors_index. The size of the
///        array is \p num_out + 1.
///
/// \param num_kernel_elements    The number of kernel elements.
///        This is the size of the first dimension of the filter, i.e.,
///        The filter shape is [num_kernel_elements, in_channels, out_channels]
///
/// \param normalize    If true then the result is normalized either by the
///        number of points (neighbors_importance is null) or by the sum of
///        the respective values in neighbors_importance.
///
template <class TReal, class TIndex>
void FillColumn(const cudaStream_t& stream,
                TReal* columns,
                int in_channels,
                TIndex begin_idx,
                TIndex end_idx,
                TIndex num_out,
                TIndex num_inp,
                const TReal* const __restrict__ inp_features,
                const TReal* const __restrict__ inp_importance,
                size_t neighbors_index_size,
                const TIndex* const __restrict__ neighbors_index,
                const int16_t* const __restrict__ neighbors_kernel_index,
                const TReal* const __restrict__ neighbors_importance,
                const int64_t* const __restrict__ neighbors_row_splits,
                const int num_kernel_elements,
                bool normalize);

template <class TReal, class TIndex>
void FillColumnTranspose(
        const cudaStream_t& stream,
        TReal* columns,
        int in_channels,
        TIndex begin_idx,
        TIndex end_idx,
        TIndex num_out,
        TIndex num_inp,
        const TReal* const __restrict__ inp_features,
        const TReal* const __restrict__ inp_neighbors_importance_sum,
        const int64_t* const __restrict__ inp_neighbors_prefix_sum,
        size_t neighbors_index_size,
        const TIndex* const __restrict__ neighbors_index,
        const int16_t* const __restrict__ neighbors_kernel_index,
        const TReal* const __restrict__ neighbors_importance,
        const int64_t* const __restrict__ neighbors_row_splits,
        const int num_kernel_elements,
        bool normalize);

}  // namespace impl
}  // namespace ml
}  // namespace open3d
