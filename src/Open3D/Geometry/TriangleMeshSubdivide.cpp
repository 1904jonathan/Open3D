// ----------------------------------------------------------------------------
// -                        Open3D: www.open3d.org                            -
// ----------------------------------------------------------------------------
// The MIT License (MIT)
//
// Copyright (c) 2019 www.open3d.org
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

#include "Open3D/Geometry/TriangleMesh.h"

#include <Eigen/Dense>
#include <queue>
#include <tuple>

#include "Open3D/Utility/Console.h"

namespace open3d {
namespace geometry {

std::shared_ptr<TriangleMesh> SubdivideMidpoint(const TriangleMesh& input,
                                                int number_of_iterations) {
    auto mesh = std::make_shared<TriangleMesh>();
    mesh->vertices_ = input.vertices_;
    mesh->vertex_colors_ = input.vertex_colors_;
    mesh->vertex_normals_ = input.vertex_normals_;
    mesh->triangles_ = input.triangles_;

    bool has_vert_normal = input.HasVertexNormals();
    bool has_vert_color = input.HasVertexColors();
    bool has_tria_normal = input.HasTriangleNormals();

    // Compute and return midpoint.
    // Also adds edge - new vertex refrence to new_verts map.
    auto SubdivideEdge =
            [&](std::unordered_map<Edge, int, utility::hash_tuple::hash<Edge>>&
                        new_verts,
                int vidx0, int vidx1) {
                int min = std::min(vidx0, vidx1);
                int max = std::max(vidx0, vidx1);
                Edge edge(min, max);
                if (new_verts.count(edge) == 0) {
                    mesh->vertices_.push_back(0.5 * (mesh->vertices_[min] +
                                                     mesh->vertices_[max]));
                    if (has_vert_normal) {
                        mesh->vertex_normals_.push_back(
                                0.5 * (mesh->vertex_normals_[min] +
                                       mesh->vertex_normals_[max]));
                    }
                    if (has_vert_color) {
                        mesh->vertex_colors_.push_back(
                                0.5 * (mesh->vertex_colors_[min] +
                                       mesh->vertex_colors_[max]));
                    }
                    int vidx01 = mesh->vertices_.size() - 1;
                    new_verts[edge] = vidx01;
                    return vidx01;
                } else {
                    return new_verts[edge];
                }
            };
    for (int iter = 0; iter < number_of_iterations; ++iter) {
        std::unordered_map<Edge, int, utility::hash_tuple::hash<Edge>>
                new_verts;
        std::vector<Eigen::Vector3i> new_triangles(4 * mesh->triangles_.size());
        for (size_t tidx = 0; tidx < mesh->triangles_.size(); ++tidx) {
            const auto& triangle = mesh->triangles_[tidx];
            int vidx0 = triangle(0);
            int vidx1 = triangle(1);
            int vidx2 = triangle(2);
            int vidx01 = SubdivideEdge(new_verts, vidx0, vidx1);
            int vidx12 = SubdivideEdge(new_verts, vidx1, vidx2);
            int vidx20 = SubdivideEdge(new_verts, vidx2, vidx0);
            new_triangles[tidx * 4 + 0] =
                    Eigen::Vector3i(vidx0, vidx01, vidx20);
            new_triangles[tidx * 4 + 1] =
                    Eigen::Vector3i(vidx01, vidx1, vidx12);
            new_triangles[tidx * 4 + 2] =
                    Eigen::Vector3i(vidx12, vidx2, vidx20);
            new_triangles[tidx * 4 + 3] =
                    Eigen::Vector3i(vidx01, vidx12, vidx20);
        }
        mesh->triangles_ = new_triangles;
    }

    if (input.HasTriangleNormals()) {
        mesh->ComputeTriangleNormals();
    }

    return mesh;
}

std::shared_ptr<TriangleMesh> SubdivideLoop(const TriangleMesh& input,
                                                int number_of_iterations) {
    typedef std::unordered_map<Edge, int, utility::hash_tuple::hash<Edge>> EdgeNewVertMap;
    typedef std::unordered_map<Edge, std::unordered_set<int>, utility::hash_tuple::hash<Edge>> EdgeTrianglesMap;
    typedef std::vector<std::unordered_set<int>> VertexNeighbours;

    auto CreateEdge = [](int vidx0, int vidx1) {
        return Edge(std::min(vidx0, vidx1), std::max(vidx0, vidx1));
    };

    bool has_vert_normal = input.HasVertexNormals();
    bool has_vert_color = input.HasVertexColors();
    bool has_tria_normal = input.HasTriangleNormals();

    auto UpdateVertex = [&](int vidx,
            const std::shared_ptr<TriangleMesh>& old_mesh,
            std::shared_ptr<TriangleMesh>& new_mesh,
            const std::unordered_set<int>& nbs,
            const EdgeTrianglesMap& edge_to_triangles) {
        // check if boundary edge and get nb vertices in that case
        std::unordered_set<int> boundary_nbs;
        for(int nb : nbs) {
            const Edge edge = CreateEdge(vidx, nb);
            if(edge_to_triangles.at(edge).size() == 1) {
                boundary_nbs.insert(nb);
            }
        }

        // in manifold meshes this should not happen
        if(boundary_nbs.size() > 2) {
            utility::PrintWarning("[SubdivideLoop] boundary edge with > 2 neighbours, maybe mesh is not manifold.\n");
        }

        double beta, alpha;
        if(boundary_nbs.size() >= 2) {
            beta = 1. / 8.;
            alpha = 1. - boundary_nbs.size() * beta;
        }
        else if(nbs.size() == 3) {
            beta = 3. / 16.;
            alpha = 1. - nbs.size() * beta;
        }
        else {
            beta = 3. / (8. * nbs.size());
            alpha = 1. - nbs.size() * beta;
        }

        new_mesh->vertices_[vidx] = alpha * old_mesh->vertices_[vidx];
        if(has_vert_normal) {
            new_mesh->vertex_normals_[vidx] = alpha * old_mesh->vertex_normals_[vidx];
        }
        if(has_vert_color) {
            new_mesh->vertex_colors_[vidx] = alpha * old_mesh->vertex_colors_[vidx];
        }

        auto Update = [&](int nb) {
            new_mesh->vertices_[vidx] += beta * old_mesh->vertices_[nb];
            if(has_vert_normal) {
                new_mesh->vertex_normals_[vidx] += beta * old_mesh->vertex_normals_[nb];
            }
            if(has_vert_color) {
                new_mesh->vertex_colors_[vidx] += beta * old_mesh->vertex_colors_[nb];
            }
        };
        if(boundary_nbs.size() >= 2) {
            for(int nb : boundary_nbs) {
                Update(nb);
            }
        }
        else {
            for(int nb : nbs) {
                Update(nb);
            }
        }

        Eigen::Vector3d nvert = new_mesh->vertices_[vidx];
        Eigen::Vector3d overt = old_mesh->vertices_[vidx];
        // printf("   from %f,%f,%f to %f,%f,%f\n", overt(0), overt(1), overt(2), nvert(0), nvert(1), nvert(2));
    };

    auto SubdivideEdge = [&](int vidx0, int vidx1,
            const std::shared_ptr<TriangleMesh>& old_mesh,
            std::shared_ptr<TriangleMesh>& new_mesh,
            EdgeNewVertMap& new_verts,
            const EdgeTrianglesMap& edge_to_triangles) {
        Edge edge = CreateEdge(vidx0, vidx1);
        // printf("    subdivide %d-%d\n", std::get<0>(edge), std::get<1>(edge));
        if (new_verts.count(edge) == 0) {
            Eigen::Vector3d new_vert = old_mesh->vertices_[vidx0] + old_mesh->vertices_[vidx1];
            Eigen::Vector3d new_normal;
            if (has_vert_normal) {
                new_normal = old_mesh->vertex_normals_[vidx0] + old_mesh->vertex_normals_[vidx1];
            }
            Eigen::Vector3d new_color;
            if (has_vert_color) {
                new_color = old_mesh->vertex_colors_[vidx0] + old_mesh->vertex_colors_[vidx1];
            }

            const auto& edge_triangles = edge_to_triangles.at(edge);
            // TODO: correct boundary handling
            if(edge_triangles.size() < 2) {
                new_vert *= 0.5;
                if(has_vert_normal) {
                    new_normal *= 0.5;
                }
                if(has_vert_color) {
                    new_color *= 0.5;
                }
            }
            else {
                new_vert *= 3. / 8.;
                if(has_vert_normal) {
                    new_normal *= 3. / 8.;
                }
                if(has_vert_color) {
                    new_color *= 3. / 8.;
                }
                int n_adjacent_trias = edge_triangles.size();
                double scale = 1. / (4. * n_adjacent_trias);
                for(int tidx : edge_triangles) {
                    const auto& tria = old_mesh->triangles_[tidx];
                    int vidx2 = (tria(0) != vidx0 && tria(0) != vidx1) ? tria(0) : ((tria(1) != vidx0 && tria(1) != vidx1) ? tria(1) : tria(2));
                    new_vert += scale * old_mesh->vertices_[vidx2];
                    if (has_vert_normal) {
                        new_normal += scale * old_mesh->vertex_normals_[vidx2];
                    }
                    if (has_vert_color) {
                        new_color += scale * old_mesh->vertex_colors_[vidx2];
                    }
                }
            }

            int vidx01 = old_mesh->vertices_.size() + new_verts.size();

            new_mesh->vertices_[vidx01] = new_vert;
            if (has_vert_normal) {
                new_mesh->vertex_normals_[vidx01] = new_normal;
            }
            if (has_vert_color) {
                new_mesh->vertex_colors_[vidx01] = new_color;
            }

            new_verts[edge] = vidx01;
            // printf("      created %f,%f,%f\n", new_vert(0), new_vert(1), new_vert(2));
            // printf("      split vidx %d (new)\n", vidx01);
            return vidx01;
        } else {
            int vidx01 = new_verts[edge];
            // printf("      split vidx %d (old)\n", vidx01);
            return vidx01;
        }
    };

    auto InsertTriangle = [&](int tidx, int vidx0, int vidx1, int vidx2,
            std::shared_ptr<TriangleMesh>& mesh,
            EdgeTrianglesMap& edge_to_triangles,
            VertexNeighbours& vertex_neighbours) {
        // printf("    insert triangle %d: %d, %d, %d | n_verts=%d\n", tidx, vidx0, vidx1, vidx2, mesh->vertices_.size());
        mesh->triangles_[tidx] = Eigen::Vector3i(vidx0, vidx1, vidx2);
        edge_to_triangles[CreateEdge(vidx0, vidx1)].insert(tidx);
        edge_to_triangles[CreateEdge(vidx1, vidx2)].insert(tidx);
        edge_to_triangles[CreateEdge(vidx2, vidx0)].insert(tidx);
        vertex_neighbours[vidx0].insert(vidx1);
        vertex_neighbours[vidx0].insert(vidx2);
        vertex_neighbours[vidx1].insert(vidx0);
        vertex_neighbours[vidx1].insert(vidx2);
        vertex_neighbours[vidx2].insert(vidx0);
        vertex_neighbours[vidx2].insert(vidx1);
    };

    // TODO: change to ptr
    EdgeTrianglesMap edge_to_triangles;
    VertexNeighbours vertex_neighbours(input.vertices_.size());
    for(size_t tidx = 0; tidx < input.triangles_.size(); ++tidx) {
        const auto& tria = input.triangles_[tidx];
        Edge e0 = CreateEdge(tria(0), tria(1));
        edge_to_triangles[e0].insert(tidx);
        Edge e1 = CreateEdge(tria(1), tria(2));
        edge_to_triangles[e1].insert(tidx);
        Edge e2 = CreateEdge(tria(2), tria(0));
        edge_to_triangles[e2].insert(tidx);

        if(edge_to_triangles[e0].size() > 2 || edge_to_triangles[e1].size() > 2 || edge_to_triangles[e2].size() > 2) {
            utility::PrintWarning("[SubdivideLoop] non-manifold edge.\n");
        }

        vertex_neighbours[tria(0)].insert(tria(1));
        vertex_neighbours[tria(0)].insert(tria(2));
        vertex_neighbours[tria(1)].insert(tria(0));
        vertex_neighbours[tria(1)].insert(tria(2));
        vertex_neighbours[tria(2)].insert(tria(0));
        vertex_neighbours[tria(2)].insert(tria(1));
    }

    auto old_mesh = std::make_shared<TriangleMesh>();
    old_mesh->vertices_ = input.vertices_;
    old_mesh->vertex_colors_ = input.vertex_colors_;
    old_mesh->vertex_normals_ = input.vertex_normals_;
    old_mesh->triangles_ = input.triangles_;

    for (int iter = 0; iter < number_of_iterations; ++iter) {
        // printf("iter = %d\n", iter);
        int n_new_vertices = old_mesh->vertices_.size() + edge_to_triangles.size();
        int n_new_triangles = 4 * old_mesh->triangles_.size();
        auto new_mesh = std::make_shared<TriangleMesh>();
        new_mesh->vertices_.resize(n_new_vertices);
        if(has_vert_normal) {
            new_mesh->vertex_normals_.resize(n_new_vertices);
        }
        if(has_vert_color) {
            new_mesh->vertex_colors_.resize(n_new_vertices);
        }
        new_mesh->triangles_.resize(n_new_triangles);
        // printf("  copied to new_mesh\n", iter);

        EdgeNewVertMap new_verts;
        EdgeTrianglesMap new_edge_to_triangles;
        VertexNeighbours new_vertex_neighbours(n_new_vertices);

        for(size_t vidx = 0; vidx < old_mesh->vertices_.size(); ++vidx) {
            // printf("  update vidx=%d\n", vidx);
            UpdateVertex(vidx, old_mesh, new_mesh, vertex_neighbours[vidx], edge_to_triangles);
        }

        for (size_t tidx = 0; tidx < old_mesh->triangles_.size(); ++tidx) {
            // printf("  update tidx=%d\n", tidx);
            const auto& triangle = old_mesh->triangles_[tidx];
            int vidx0 = triangle(0);
            int vidx1 = triangle(1);
            int vidx2 = triangle(2);

            int vidx01 = SubdivideEdge(vidx0, vidx1, old_mesh, new_mesh, new_verts, edge_to_triangles);
            int vidx12 = SubdivideEdge(vidx1, vidx2, old_mesh, new_mesh, new_verts, edge_to_triangles);
            int vidx20 = SubdivideEdge(vidx2, vidx0, old_mesh, new_mesh, new_verts, edge_to_triangles);

            InsertTriangle(tidx * 4 + 0, vidx0, vidx01, vidx20, new_mesh, new_edge_to_triangles, new_vertex_neighbours);
            InsertTriangle(tidx * 4 + 1, vidx01, vidx1, vidx12, new_mesh, new_edge_to_triangles, new_vertex_neighbours);
            InsertTriangle(tidx * 4 + 2, vidx12, vidx2, vidx20, new_mesh, new_edge_to_triangles, new_vertex_neighbours);
            InsertTriangle(tidx * 4 + 3, vidx01, vidx12, vidx20, new_mesh, new_edge_to_triangles, new_vertex_neighbours);
        }

        old_mesh = new_mesh;
        edge_to_triangles = new_edge_to_triangles;
        vertex_neighbours = new_vertex_neighbours;
    }

    if (input.HasTriangleNormals()) {
        old_mesh->ComputeTriangleNormals();
    }

    return old_mesh;
}

}  // namespace geometry
}  // namespace open3d
