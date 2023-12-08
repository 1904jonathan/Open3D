# ----------------------------------------------------------------------------
# -                        Open3D: www.open3d.org                            -
# ----------------------------------------------------------------------------
# Copyright (c) 2018-2023 www.open3d.org
# SPDX-License-Identifier: MIT
# ----------------------------------------------------------------------------

import open3d as o3d
import open3d.core as o3c
import numpy as np
import pytest
import pickle
import tempfile

import sys
import os
from copy import deepcopy
from typing import List

sys.path.append(os.path.dirname(os.path.realpath(__file__)) + "/../..")
from open3d_test import list_devices


def test_clip_plane():
    cube = o3d.t.geometry.TriangleMesh.from_legacy(
        o3d.geometry.TriangleMesh.create_box())
    clipped_cube = cube.clip_plane(point=[0.5, 0, 0], normal=[1, 0, 0])
    assert clipped_cube.vertex.positions.shape == (12, 3)
    assert clipped_cube.triangle.indices.shape == (14, 3)


def test_slice_plane():
    box = o3d.t.geometry.TriangleMesh.create_box()
    slices = box.slice_plane([0, 0.5, 0], [1, 1, 1], [-0.1, 0, 0.1])
    assert slices.point.positions.shape == (9, 3)
    assert slices.line.indices.shape == (9, 2)


@pytest.mark.parametrize("device", list_devices())
def test_create_box(device):
    # Test with default parameters.
    box_default = o3d.t.geometry.TriangleMesh.create_box(device=device)

    vertex_positions_default = o3c.Tensor(
        [[0.0, 0.0, 0.0], [1.0, 0.0, 0.0], [0.0, 0.0, 1.0], [1.0, 0.0, 1.0],
         [0.0, 1.0, 0.0], [1.0, 1.0, 0.0], [0.0, 1.0, 1.0], [1.0, 1.0, 1.0]],
        o3c.float32, device)

    triangle_indices_default = o3c.Tensor(
        [[4, 7, 5], [4, 6, 7], [0, 2, 4], [2, 6, 4], [0, 1, 2], [1, 3, 2],
         [1, 5, 7], [1, 7, 3], [2, 3, 7], [2, 7, 6], [0, 4, 1], [1, 4, 5]],
        o3c.int64, device)

    assert box_default.vertex.positions.allclose(vertex_positions_default)
    assert box_default.triangle.indices.allclose(triangle_indices_default)

    # Test with custom parameters.
    box_custom = o3d.t.geometry.TriangleMesh.create_box(2, 3, 4, o3c.float64,
                                                        o3c.int32, device)

    vertex_positions_custom = o3c.Tensor(
        [[0.0, 0.0, 0.0], [2.0, 0.0, 0.0], [0.0, 0.0, 4.0], [2.0, 0.0, 4.0],
         [0.0, 3.0, 0.0], [2.0, 3.0, 0.0], [0.0, 3.0, 4.0], [2.0, 3.0, 4.0]],
        o3c.float64, device)

    triangle_indices_custom = o3c.Tensor(
        [[4, 7, 5], [4, 6, 7], [0, 2, 4], [2, 6, 4], [0, 1, 2], [1, 3, 2],
         [1, 5, 7], [1, 7, 3], [2, 3, 7], [2, 7, 6], [0, 4, 1], [1, 4, 5]],
        o3c.int32, device)

    assert box_custom.vertex.positions.allclose(vertex_positions_custom)
    assert box_custom.triangle.indices.allclose(triangle_indices_custom)


@pytest.mark.parametrize("device", list_devices())
def test_create_sphere(device):
    # Test with custom parameters.
    sphere_custom = o3d.t.geometry.TriangleMesh.create_sphere(
        1, 3, o3c.float64, o3c.int32, device)

    vertex_positions_custom = o3c.Tensor(
        [[0.0, 0.0, 1.0], [0.0, 0.0, -1.0], [0.866025, 0, 0.5],
         [0.433013, 0.75, 0.5], [-0.433013, 0.75, 0.5], [-0.866025, 0.0, 0.5],
         [-0.433013, -0.75, 0.5], [0.433013, -0.75, 0.5], [0.866025, 0.0, -0.5],
         [0.433013, 0.75, -0.5], [-0.433013, 0.75, -0.5],
         [-0.866025, 0.0, -0.5], [-0.433013, -0.75, -0.5],
         [0.433013, -0.75, -0.5]], o3c.float64, device)

    triangle_indices_custom = o3c.Tensor(
        [[0, 2, 3], [1, 9, 8], [0, 3, 4], [1, 10, 9], [0, 4, 5], [1, 11, 10],
         [0, 5, 6], [1, 12, 11], [0, 6, 7], [1, 13, 12], [0, 7, 2], [1, 8, 13],
         [8, 3, 2], [8, 9, 3], [9, 4, 3], [9, 10, 4], [10, 5, 4], [10, 11, 5],
         [11, 6, 5], [11, 12, 6], [12, 7, 6], [12, 13, 7], [13, 2, 7],
         [13, 8, 2]], o3c.int32, device)

    assert sphere_custom.vertex.positions.allclose(vertex_positions_custom)
    assert sphere_custom.triangle.indices.allclose(triangle_indices_custom)


@pytest.mark.parametrize("device", list_devices())
def test_create_tetrahedron(device):
    # Test with custom parameters.
    tetrahedron_custom = o3d.t.geometry.TriangleMesh.create_tetrahedron(
        2, o3c.float64, o3c.int32, device)

    vertex_positions_custom = o3c.Tensor(
        [[1.88562, 0.0, -0.666667], [-0.942809, 1.63299, -0.666667],
         [-0.942809, -1.63299, -0.666667], [0.0, 0.0, 2]], o3c.float64, device)

    triangle_indices_custom = o3c.Tensor(
        [[0, 2, 1], [0, 3, 2], [0, 1, 3], [1, 2, 3]], o3c.int32, device)

    assert tetrahedron_custom.vertex.positions.allclose(vertex_positions_custom)
    assert tetrahedron_custom.triangle.indices.allclose(triangle_indices_custom)


@pytest.mark.parametrize("device", list_devices())
def test_create_octahedron(device):
    # Test with custom parameters.
    octahedron_custom = o3d.t.geometry.TriangleMesh.create_octahedron(
        2, o3c.float64, o3c.int32, device)

    vertex_positions_custom = o3c.Tensor(
        [[2.0, 0.0, 0.0], [0.0, 2.0, 0.0], [0.0, 0.0, 2.0], [-2.0, 0.0, 0.0],
         [0.0, -2.0, 0.0], [0.0, 0.0, -2.0]], o3c.float64, device)

    triangle_indices_custom = o3c.Tensor(
        [[0, 1, 2], [1, 3, 2], [3, 4, 2], [4, 0, 2], [0, 5, 1], [1, 5, 3],
         [3, 5, 4], [4, 5, 0]], o3c.int32, device)

    assert octahedron_custom.vertex.positions.allclose(vertex_positions_custom)
    assert octahedron_custom.triangle.indices.allclose(triangle_indices_custom)


@pytest.mark.parametrize("device", list_devices())
def test_create_icosahedron(device):
    # Test with custom parameters.
    icosahedron_custom = o3d.t.geometry.TriangleMesh.create_icosahedron(
        2, o3c.float64, o3c.int32, device)

    vertex_positions_custom = o3c.Tensor(
        [[-2.0, 0.0, 3.23607], [2.0, 0.0, 3.23607], [2.0, 0.0, -3.23607],
         [-2.0, 0.0, -3.23607], [0.0, -3.23607, 2.0], [0.0, 3.23607, 2.0],
         [0.0, 3.23607, -2.0], [0.0, -3.23607, -2.0], [-3.23607, -2.0, 0.0],
         [3.23607, -2.0, 0.0], [3.23607, 2.0, 0.0], [-3.23607, 2.0, 0.0]],
        o3c.float64, device)

    triangle_indices_custom = o3c.Tensor(
        [[0, 4, 1], [0, 1, 5], [1, 4, 9], [1, 9, 10], [1, 10, 5], [0, 8, 4],
         [0, 11, 8], [0, 5, 11], [5, 6, 11], [5, 10, 6], [4, 8, 7], [4, 7, 9],
         [3, 6, 2], [3, 2, 7], [2, 6, 10], [2, 10, 9], [2, 9, 7], [3, 11, 6],
         [3, 8, 11], [3, 7, 8]], o3c.int32, device)

    assert icosahedron_custom.vertex.positions.allclose(vertex_positions_custom)
    assert icosahedron_custom.triangle.indices.allclose(triangle_indices_custom)


@pytest.mark.parametrize("device", list_devices())
def test_create_cylinder(device):
    # Test with custom parameters.
    cylinder_custom = o3d.t.geometry.TriangleMesh.create_cylinder(
        1, 2, 3, 3, o3c.float64, o3c.int32, device)

    vertex_positions_custom = o3c.Tensor(
        [[0.0, 0.0, 1.0], [0.0, 0.0, -1.0], [1.0, 0.0, 1.0],
         [-0.5, 0.866025, 1.0], [-0.5, -0.866025, 1.0], [1.0, 0.0, 0.333333],
         [-0.5, 0.866025, 0.333333], [-0.5, -0.866025, 0.333333],
         [1.0, 0.0, -0.333333], [-0.5, 0.866025, -0.333333],
         [-0.5, -0.866025, -0.333333], [1.0, 0.0, -1.0], [-0.5, 0.866025, -1.0],
         [-0.5, -0.866025, -1.0]], o3c.float64, device)

    triangle_indices_custom = o3c.Tensor(
        [[0, 2, 3], [1, 12, 11], [0, 3, 4], [1, 13, 12], [0, 4, 2], [1, 11, 13],
         [5, 3, 2], [5, 6, 3], [6, 4, 3], [6, 7, 4], [7, 2, 4], [7, 5, 2],
         [8, 6, 5], [8, 9, 6], [9, 7, 6], [9, 10, 7], [10, 5, 7], [10, 8, 5],
         [11, 9, 8], [11, 12, 9], [12, 10, 9], [12, 13, 10], [13, 8, 10],
         [13, 11, 8]], o3c.int32, device)

    assert cylinder_custom.vertex.positions.allclose(vertex_positions_custom)
    assert cylinder_custom.triangle.indices.allclose(triangle_indices_custom)


@pytest.mark.parametrize("device", list_devices())
def test_create_cone(device):
    # Test with custom parameters.
    cone_custom = o3d.t.geometry.TriangleMesh.create_cone(
        2, 4, 3, 2, o3c.float64, o3c.int32, device)

    vertex_positions_custom = o3c.Tensor(
        [[0.0, 0.0, 0.0], [0.0, 0.0, 4.0], [2.0, 0.0, 0.0],
         [-1.0, 1.73205, 0.0], [-1.0, -1.73205, 0.0], [1.0, 0.0, 2.0],
         [-0.5, 0.866025, 2], [-0.5, -0.866025, 2]], o3c.float64, device)

    triangle_indices_custom = o3c.Tensor(
        [[0, 3, 2], [1, 5, 6], [0, 4, 3], [1, 6, 7], [0, 2, 4], [1, 7, 5],
         [6, 2, 3], [6, 5, 2], [7, 3, 4], [7, 6, 3], [5, 4, 2], [5, 7, 4]],
        o3c.int32, device)

    assert cone_custom.vertex.positions.allclose(vertex_positions_custom)
    assert cone_custom.triangle.indices.allclose(triangle_indices_custom)


@pytest.mark.parametrize("device", list_devices())
def test_create_torus(device):
    # Test with custom parameters.
    torus_custom = o3d.t.geometry.TriangleMesh.create_torus(
        2, 1, 6, 3, o3c.float64, o3c.int32, device)

    vertex_positions_custom = o3c.Tensor(
        [[3.0, 0.0, 0.0], [1.5, 0.0, 0.866025], [1.5, 0.0, -0.866025],
         [1.5, 2.59808, 0.0], [0.75, 1.29904, 0.866025],
         [0.75, 1.29904, -0.866025], [-1.5, 2.59808, 0],
         [-0.75, 1.29904, 0.866025], [-0.75, 1.29904, -0.866025],
         [-3.0, 0.0, 0.0], [-1.5, 0.0, 0.866025], [-1.5, 0.0, -0.866025],
         [-1.5, -2.59808, 0.0], [-0.75, -1.29904, 0.866025],
         [-0.75, -1.29904, -0.866025], [1.5, -2.59808, 0.0],
         [0.75, -1.29904, 0.866025], [0.75, -1.29904, -0.866025]], o3c.float64,
        device)

    triangle_indices_custom = o3c.Tensor(
        [[3, 4, 0], [0, 4, 1], [4, 5, 1], [1, 5, 2], [5, 3, 2], [2, 3, 0],
         [6, 7, 3], [3, 7, 4], [7, 8, 4], [4, 8, 5], [8, 6, 5], [5, 6, 3],
         [9, 10, 6], [6, 10, 7], [10, 11, 7], [7, 11, 8], [11, 9, 8], [8, 9, 6],
         [12, 13, 9], [9, 13, 10], [13, 14, 10], [10, 14, 11], [14, 12, 11],
         [11, 12, 9], [15, 16, 12], [12, 16, 13], [16, 17, 13], [13, 17, 14],
         [17, 15, 14], [14, 15, 12], [0, 1, 15], [15, 1, 16], [1, 2, 16],
         [16, 2, 17], [2, 0, 17], [17, 0, 15]], o3c.int32, device)

    assert torus_custom.vertex.positions.allclose(vertex_positions_custom)
    assert torus_custom.triangle.indices.allclose(triangle_indices_custom)


@pytest.mark.parametrize("device", list_devices())
def test_create_arrow(device):
    # Test with custom parameters.
    arrow_custom = o3d.t.geometry.TriangleMesh.create_arrow(
        1, 2, 4, 2, 4, 1, 1, o3c.float64, o3c.int32, device)

    vertex_positions_custom = o3c.Tensor(
        [[0.0, 0.0, 4.0], [0.0, 0.0, 0.0], [1.0, 0.0, 4.0], [0.0, 1.0, 4.0],
         [-1.0, 0.0, 4.0], [0.0, -1.0, 4.0], [1.0, 0.0, 0.0], [0.0, 1.0, 0.0],
         [-1.0, 0.0, 0.0], [0.0, -1.0, 0.0], [0.0, 0.0, 4.0], [0.0, 0.0, 6.0],
         [2.0, 0.0, 4.0], [0.0, 2.0, 4.0], [-2.0, 0.0, 4.0], [0.0, -2.0, 4.0]],
        o3c.float64, device)

    triangle_indices_custom = o3c.Tensor(
        [[0, 2, 3], [1, 7, 6], [0, 3, 4], [1, 8, 7], [0, 4, 5], [1, 9, 8],
         [0, 5, 2], [1, 6, 9], [6, 3, 2], [6, 7, 3], [7, 4, 3], [7, 8, 4],
         [8, 5, 4], [8, 9, 5], [9, 2, 5], [9, 6, 2], [10, 13, 12], [11, 12, 13],
         [10, 14, 13], [11, 13, 14], [10, 15, 14], [11, 14, 15], [10, 12, 15],
         [11, 15, 12]], o3c.int32, device)

    assert arrow_custom.vertex.positions.allclose(vertex_positions_custom)
    assert arrow_custom.triangle.indices.allclose(triangle_indices_custom)


@pytest.mark.parametrize("device", list_devices())
def test_create_mobius(device):
    # Test with custom parameters.
    mobius_custom = o3d.t.geometry.TriangleMesh.create_mobius(
        10, 2, 1, 1, 1, 1, 1, o3c.float64, o3c.int32, device)

    vertex_positions_custom = o3c.Tensor(
        [[0.5, 0.0, 0.0], [1.5, 0.0, 0.0], [0.424307, 0.308277, -0.154508],
         [1.19373, 0.867294, 0.154508], [0.184017, 0.566346, -0.293893],
         [0.434017, 1.33577, 0.293893], [-0.218199, 0.671548, -0.404508],
         [-0.399835, 1.23057, 0.404508], [-0.684017, 0.496967, -0.475528],
         [-0.934017, 0.678603, 0.475528], [-1.0, 0.0, -0.5], [-1.0, 0.0, 0.5],
         [-0.934017, -0.678603, -0.475528], [-0.684017, -0.496967, 0.475528],
         [-0.399835, -1.23057, -0.404508], [-0.218199, -0.671548, 0.404508],
         [0.434017, -1.33577, -0.293893], [0.184017, -0.566346, 0.293893],
         [1.19373, -0.867294, -0.154508], [0.424307, -0.308277, 0.154508]],
        o3c.float64, device)

    triangle_indices_custom = o3c.Tensor(
        [[0, 3, 1], [0, 2, 3], [3, 2, 4], [3, 4, 5], [4, 7, 5], [4, 6, 7],
         [7, 6, 8], [7, 8, 9], [8, 11, 9], [8, 10, 11], [11, 10, 12],
         [11, 12, 13], [12, 15, 13], [12, 14, 15], [15, 14, 16], [15, 16, 17],
         [16, 19, 17], [16, 18, 19], [18, 19, 1], [1, 19, 0]], o3c.int32,
        device)

    assert mobius_custom.vertex.positions.allclose(vertex_positions_custom)
    assert mobius_custom.triangle.indices.allclose(triangle_indices_custom)


def test_create_text():
    mesh = o3d.t.geometry.TriangleMesh.create_text('Open3D', depth=1)
    assert mesh.vertex.positions.shape == (624, 3)
    assert mesh.triangle.indices.shape == (936, 3)


def test_simplify_quadric_decimation():
    cube = o3d.t.geometry.TriangleMesh.from_legacy(
        o3d.geometry.TriangleMesh.create_box().subdivide_midpoint(3))

    # Chose reduction factor such that we get 12 faces.
    target_reduction = 1 - (12 / cube.triangle.indices.shape[0])
    simplified = cube.simplify_quadric_decimation(
        target_reduction=target_reduction)

    assert simplified.vertex.positions.shape == (8, 3)
    assert simplified.triangle.indices.shape == (12, 3)


def test_boolean_operations():
    box = o3d.geometry.TriangleMesh.create_box()
    box = o3d.t.geometry.TriangleMesh.from_legacy(box)
    sphere = o3d.geometry.TriangleMesh.create_sphere(0.8)
    sphere = o3d.t.geometry.TriangleMesh.from_legacy(sphere)
    # check input sphere
    assert sphere.vertex.positions.shape == (762, 3)
    assert sphere.triangle.indices.shape == (1520, 3)

    ans = box.boolean_union(sphere)
    assert ans.vertex.positions.shape == (730, 3)
    assert ans.triangle.indices.shape == (1384, 3)

    ans = box.boolean_intersection(sphere)
    assert ans.vertex.positions.shape == (154, 3)
    assert ans.triangle.indices.shape == (232, 3)

    ans = box.boolean_difference(sphere)
    assert ans.vertex.positions.shape == (160, 3)
    assert ans.triangle.indices.shape == (244, 3)


def test_hole_filling():
    sphere = o3d.geometry.TriangleMesh.create_sphere(1.0)
    sphere = o3d.t.geometry.TriangleMesh.from_legacy(sphere)
    clipped = sphere.clip_plane([0.8, 0, 0], [1, 0, 0])
    assert not clipped.to_legacy().is_watertight()
    filled = clipped.fill_holes()
    assert filled.to_legacy().is_watertight()


def test_uvatlas():
    box = o3d.t.geometry.TriangleMesh.create_box()
    box.compute_uvatlas()
    assert box.triangle['texture_uvs'].shape == (12, 3, 2)


def test_bake_vertex_attr_textures():
    desired = np.array([
        [[0., 0., 0.], [0., 0., 0.], [1., 0.25, 0.75], [1., 0.75, 0.75],
         [0., 0., 0.], [0., 0., 0.], [0., 0., 0.], [0., 0., 0.]],
        [[0., 0., 0.], [0., 0., 0.], [1., 0.25, 0.25], [1., 0.75, 0.25],
         [0., 0., 0.], [0., 0., 0.], [0., 0., 0.], [0., 0., 0.]],
        [[0.75, 0., 0.75], [0.75, 0., 0.25], [0.75, 0.25, 0.], [0.75, 0.75, 0.],
         [0.75, 1., 0.25], [0.75, 1., 0.75], [0., 0., 0.], [0., 0., 0.]],
        [[0.25, 0., 0.75], [0.25, 0., 0.25], [0.25, 0.25, 0.], [0.25, 0.75, 0.],
         [0.25, 1., 0.25], [0.25, 1., 0.75], [0., 0., 0.], [0., 0., 0.]],
        [[0., 0., 0.], [0., 0., 0.], [0., 0.25, 0.25], [0., 0.75, 0.25],
         [0., 0., 0.], [0., 0., 0.], [0., 0., 0.], [0., 0., 0.]],
        [[0., 0., 0.], [0., 0., 0.], [0., 0.25, 0.75], [0., 0.75, 0.75],
         [0., 0., 0.], [0., 0., 0.], [0., 0., 0.], [0., 0., 0.]],
        [[0., 0., 0.], [0., 0., 0.], [0.25, 0.25, 1.], [0.25, 0.75, 1.],
         [0., 0., 0.], [0., 0., 0.], [0., 0., 0.], [0., 0., 0.]],
        [[0., 0., 0.], [0., 0., 0.], [0.75, 0.25, 1.], [0.75, 0.75, 1.],
         [0., 0., 0.], [0., 0., 0.], [0., 0., 0.], [0., 0., 0.]]
    ],
                       dtype=np.float32)

    box = o3d.geometry.TriangleMesh.create_box(create_uv_map=True)
    box = o3d.t.geometry.TriangleMesh.from_legacy(box)
    textures = box.bake_vertex_attr_textures(8, {'positions'}, margin=0.1)

    np.testing.assert_allclose(textures['positions'].numpy(), desired)


def test_bake_triangle_attr_textures():
    desired = np.array(
        [[-1, -1, 7, 7, -1, -1, -1, -1], [-1, -1, 7, 6, -1, -1, -1, -1],
         [5, 5, 10, 11, 0, 0, -1, -1], [5, 4, 10, 10, 0, 1, -1, -1],
         [-1, -1, 2, 2, -1, -1, -1, -1], [-1, -1, 2, 3, -1, -1, -1, -1],
         [-1, -1, 8, 9, -1, -1, -1, -1], [-1, -1, 8, 8, -1, -1, -1, -1]],
        dtype=np.int64)

    box = o3d.geometry.TriangleMesh.create_box(create_uv_map=True)
    box = o3d.t.geometry.TriangleMesh.from_legacy(box)
    box.triangle.index = np.arange(box.triangle.indices.shape[0])
    # shift the uvs to avoid pixel centers exactly at triangle boundaries.
    box.triangle.texture_uvs[:, :, 0] += 0.01

    textures = box.bake_triangle_attr_textures(8, {'index'},
                                               margin=0.1,
                                               fill=-1)
    np.testing.assert_equal(textures['index'].numpy(), desired)


def test_extrude_rotation():
    mesh = o3d.t.geometry.TriangleMesh([[1, 1, 0], [0.7, 1, 0], [1, 0.7, 0]],
                                       [[0, 1, 2]])
    ans = mesh.extrude_rotation(3 * 360, [0, 1, 0],
                                resolution=3 * 16,
                                translation=2)
    assert ans.vertex.positions.shape == (147, 3)
    assert ans.triangle.indices.shape == (290, 3)


def test_extrude_linear():
    triangle = o3d.t.geometry.TriangleMesh(
        [[1.0, 1.0, 0.0], [0, 1, 0], [1, 0, 0]], [[0, 1, 2]])
    ans = triangle.extrude_linear([0, 0, 1])
    assert ans.vertex.positions.shape == (6, 3)
    assert ans.triangle.indices.shape == (8, 3)


@pytest.mark.parametrize("device", list_devices())
def test_remove_duplicate_vertices1(device):
    """Test Remove duplicates works for float32 coords and int32 indices"""
    dtype_g = o3d.core.float32
    dtype_i = o3d.core.int32
    mesh = o3d.t.geometry.TriangleMesh(device)
    mesh.vertex.positions = o3d.core.Tensor([[0.0, 0.0, 0.0],
                                             [1.0, 0.0, 0.0],
                                             [0.0, 0.0, 1.0],
                                             [1.0, 0.0, 1.0],
                                             [0.0, 1.0, 0.0],
                                             [1.0, 1.0, 0.0],
                                             [0.0, 1.0, 1.0],
                                             [1.0, 0.0, 0.0],
                                             [1.0, 1.0, 1.0]], dtype=dtype_g)
    mesh.triangle.indices = o3d.core.Tensor([[4, 8, 5],
                                             [4, 6, 8],
                                             [0, 2, 4],
                                             [2, 6, 4],
                                             [0, 1, 2],
                                             [1, 3, 2],
                                             [7, 5, 8],
                                             [7, 8, 3],
                                             [2, 3, 8],
                                             [2, 8, 6],
                                             [0, 4, 1],
                                             [1, 4, 5]], dtype=dtype_i)
    mesh.remove_duplicate_vertices()
    expected_pos = o3d.core.Tensor([[0., 0., 0.],
                                    [1., 0., 0.],
                                    [0., 0., 1.],
                                    [1., 0., 1.],
                                    [0., 1., 0.],
                                    [1., 1., 0.],
                                    [0., 1., 1.],
                                    [1., 1., 1.]], dtype=dtype_g)
    expected_ind = o3d.core.Tensor([[4, 7, 5],
                                    [4, 6, 7],
                                    [0, 2, 4],
                                    [2, 6, 4],
                                    [0, 1, 2],
                                    [1, 3, 2],
                                    [1, 5, 7],
                                    [1, 7, 3],
                                    [2, 3, 7],
                                    [2, 7, 6],
                                    [0, 4, 1],
                                    [1, 4, 5]], dtype=dtype_i)

    assert(mesh.vertex.positions.allclose(expected_pos))
    assert(mesh.triangle.indices.allclose(expected_ind))


@pytest.mark.parametrize("device", list_devices())
def test_remove_duplicate_vertices2(device):
    """Test Remove duplicates works for float32 coords and int64 indices"""
    dtype_g = o3d.core.float32
    dtype_i = o3d.core.int64
    mesh = o3d.t.geometry.TriangleMesh(device)
    mesh.vertex.positions = o3d.core.Tensor([[0.0, 0.0, 0.0],
                                             [1.0, 0.0, 0.0],
                                             [0.0, 0.0, 1.0],
                                             [1.0, 0.0, 1.0],
                                             [0.0, 1.0, 0.0],
                                             [1.0, 1.0, 0.0],
                                             [0.0, 1.0, 1.0],
                                             [1.0, 0.0, 0.0],
                                             [1.0, 1.0, 1.0]], dtype=dtype_g)
    mesh.triangle.indices = o3d.core.Tensor([[4, 8, 5],
                                             [4, 6, 8],
                                             [0, 2, 4],
                                             [2, 6, 4],
                                             [0, 1, 2],
                                             [1, 3, 2],
                                             [7, 5, 8],
                                             [7, 8, 3],
                                             [2, 3, 8],
                                             [2, 8, 6],
                                             [0, 4, 1],
                                             [1, 4, 5]], dtype=dtype_i)
    mesh.remove_duplicate_vertices()
    expected_pos = o3d.core.Tensor([[0., 0., 0.],
                                    [1., 0., 0.],
                                    [0., 0., 1.],
                                    [1., 0., 1.],
                                    [0., 1., 0.],
                                    [1., 1., 0.],
                                    [0., 1., 1.],
                                    [1., 1., 1.]], dtype=dtype_g)
    expected_ind = o3d.core.Tensor([[4, 7, 5],
                                    [4, 6, 7],
                                    [0, 2, 4],
                                    [2, 6, 4],
                                    [0, 1, 2],
                                    [1, 3, 2],
                                    [1, 5, 7],
                                    [1, 7, 3],
                                    [2, 3, 7],
                                    [2, 7, 6],
                                    [0, 4, 1],
                                    [1, 4, 5]], dtype=dtype_i)

    assert(mesh.vertex.positions.allclose(expected_pos))
    assert(mesh.triangle.indices.allclose(expected_ind))


@pytest.mark.parametrize("device", list_devices())
def test_remove_duplicate_vertices3(device):
    """Test Remove duplicates works for float64 coords and int32 indices"""
    dtype_g = o3d.core.float64
    dtype_i = o3d.core.int32
    mesh = o3d.t.geometry.TriangleMesh(device)
    mesh.vertex.positions = o3d.core.Tensor([[0.0, 0.0, 0.0],
                                             [1.0, 0.0, 0.0],
                                             [0.0, 0.0, 1.0],
                                             [1.0, 0.0, 1.0],
                                             [0.0, 1.0, 0.0],
                                             [1.0, 1.0, 0.0],
                                             [0.0, 1.0, 1.0],
                                             [1.0, 0.0, 0.0],
                                             [1.0, 1.0, 1.0]], dtype=dtype_g)
    mesh.triangle.indices = o3d.core.Tensor([[4, 8, 5],
                                             [4, 6, 8],
                                             [0, 2, 4],
                                             [2, 6, 4],
                                             [0, 1, 2],
                                             [1, 3, 2],
                                             [7, 5, 8],
                                             [7, 8, 3],
                                             [2, 3, 8],
                                             [2, 8, 6],
                                             [0, 4, 1],
                                             [1, 4, 5]], dtype=dtype_i)
    mesh.remove_duplicate_vertices()
    expected_pos = o3d.core.Tensor([[0., 0., 0.],
                                    [1., 0., 0.],
                                    [0., 0., 1.],
                                    [1., 0., 1.],
                                    [0., 1., 0.],
                                    [1., 1., 0.],
                                    [0., 1., 1.],
                                    [1., 1., 1.]], dtype=dtype_g)
    expected_ind = o3d.core.Tensor([[4, 7, 5],
                                    [4, 6, 7],
                                    [0, 2, 4],
                                    [2, 6, 4],
                                    [0, 1, 2],
                                    [1, 3, 2],
                                    [1, 5, 7],
                                    [1, 7, 3],
                                    [2, 3, 7],
                                    [2, 7, 6],
                                    [0, 4, 1],
                                    [1, 4, 5]], dtype=dtype_i)

    print(f"DEBUG V: {mesh.vertex.positions}")
    assert(mesh.vertex.positions.allclose(expected_pos))
    print(f"DEBUG T: {mesh.triangle.indices}")
    assert(mesh.triangle.indices.allclose(expected_ind))


@pytest.mark.parametrize("device", list_devices())
def test_remove_duplicate_vertices4(device):
    """Test Remove duplicates works for float64 coords and int64 indices"""
    dtype_g = o3d.core.float64
    dtype_i = o3d.core.int64
    mesh = o3d.t.geometry.TriangleMesh(device)
    mesh.vertex.positions = o3d.core.Tensor([[0.0, 0.0, 0.0],
                                             [1.0, 0.0, 0.0],
                                             [0.0, 0.0, 1.0],
                                             [1.0, 0.0, 1.0],
                                             [0.0, 1.0, 0.0],
                                             [1.0, 1.0, 0.0],
                                             [0.0, 1.0, 1.0],
                                             [1.0, 0.0, 0.0],
                                             [1.0, 1.0, 1.0]], dtype=dtype_g)
    mesh.triangle.indices = o3d.core.Tensor([[4, 8, 5],
                                             [4, 6, 8],
                                             [0, 2, 4],
                                             [2, 6, 4],
                                             [0, 1, 2],
                                             [1, 3, 2],
                                             [7, 5, 8],
                                             [7, 8, 3],
                                             [2, 3, 8],
                                             [2, 8, 6],
                                             [0, 4, 1],
                                             [1, 4, 5]], dtype=dtype_i)
    mesh.remove_duplicate_vertices()
    expected_pos = o3d.core.Tensor([[0., 0., 0.],
                                    [1., 0., 0.],
                                    [0., 0., 1.],
                                    [1., 0., 1.],
                                    [0., 1., 0.],
                                    [1., 1., 0.],
                                    [0., 1., 1.],
                                    [1., 1., 1.]], dtype=dtype_g)
    expected_ind = o3d.core.Tensor([[4, 7, 5],
                                    [4, 6, 7],
                                    [0, 2, 4],
                                    [2, 6, 4],
                                    [0, 1, 2],
                                    [1, 3, 2],
                                    [1, 5, 7],
                                    [1, 7, 3],
                                    [2, 3, 7],
                                    [2, 7, 6],
                                    [0, 4, 1],
                                    [1, 4, 5]], dtype=dtype_i)

    print(f"DEBUG V: {mesh.vertex.positions}")
    assert(mesh.vertex.positions.allclose(expected_pos))
    print(f"DEBUG T: {mesh.triangle.indices}")
    assert(mesh.triangle.indices.allclose(expected_ind))


@pytest.mark.parametrize("device", list_devices())
def test_remove_duplicate_vertices5(device):
    """
    Test that the vertex attribute colors are updated properly
    and triangle color attribute are all preserved correctly
    after remove duplicate operation.
    """
    dtype_g = o3d.core.float64
    dtype_i = o3d.core.int32
    mesh = o3d.t.geometry.TriangleMesh(device)
    mesh.vertex.positions = o3d.core.Tensor([[0.0, 0.0, 0.0],
                                             [1.0, 0.0, 0.0],
                                             [0.0, 0.0, 1.0],
                                             [1.0, 0.0, 1.0],
                                             [0.0, 1.0, 0.0],
                                             [1.0, 1.0, 0.0],
                                             [0.0, 1.0, 1.0],
                                             [1.0, 0.0, 0.0],
                                             [1.0, 1.0, 1.0]], dtype=dtype_g)
    mesh.vertex.colors = o3d.core.Tensor([[10, 20, 30],
                                          [20, 30, 40],
                                          [30, 40, 50],
                                          [40, 50, 60],
                                          [50, 60, 70],
                                          [60, 70, 80],
                                          [70, 80, 90],
                                          [20, 30, 40],
                                          [80, 90, 100]], dtype=dtype_g)
    expected_vertex_colors = o3d.core.Tensor([[10, 20, 30],
                                          [20, 30, 40],
                                          [30, 40, 50],
                                          [40, 50, 60],
                                          [50, 60, 70],
                                          [60, 70, 80],
                                          [70, 80, 90],
                                          [80, 90, 100]], dtype=dtype_g)
    mesh.triangle.indices = o3d.core.Tensor([[4, 8, 5],
                                             [4, 6, 8],
                                             [0, 2, 4],
                                             [2, 6, 4],
                                             [0, 1, 2],
                                             [1, 3, 2],
                                             [7, 5, 8],
                                             [7, 8, 3],
                                             [2, 3, 8],
                                             [2, 8, 6],
                                             [0, 4, 1],
                                             [1, 4, 5]], dtype=dtype_i)
    expected_tri_colors = o3d.core.Tensor([[100, 11, 20],
                                           [110, 12, 32],
                                           [120, 13, 42],
                                           [130, 14, 52],
                                           [140, 15, 62],
                                           [150, 16, 72],
                                           [160, 17, 82],
                                           [170, 18, 92],
                                           [180, 19, 102],
                                           [190, 20, 112],
                                           [200, 21, 122],
                                           [210, 22, 132]], dtype=dtype_g)
    mesh.triangle.colors = expected_tri_colors
    mesh.remove_duplicate_vertices()
    expected_pos = o3d.core.Tensor([[0., 0., 0.],
                                    [1., 0., 0.],
                                    [0., 0., 1.],
                                    [1., 0., 1.],
                                    [0., 1., 0.],
                                    [1., 1., 0.],
                                    [0., 1., 1.],
                                    [1., 1., 1.]], dtype=dtype_g)
    expected_ind = o3d.core.Tensor([[4, 7, 5],
                                    [4, 6, 7],
                                    [0, 2, 4],
                                    [2, 6, 4],
                                    [0, 1, 2],
                                    [1, 3, 2],
                                    [1, 5, 7],
                                    [1, 7, 3],
                                    [2, 3, 7],
                                    [2, 7, 6],
                                    [0, 4, 1],
                                    [1, 4, 5]], dtype=dtype_i)
    print(f"DEBUG V: {mesh.vertex.positions}")
    assert(mesh.vertex.positions.allclose(expected_pos))
    print(f"DEBUG VC: {mesh.vertex.colors}")
    assert(mesh.vertex.colors.allclose(expected_vertex_colors))
    print(f"DEBUG T: {mesh.triangle.indices}")
    assert(mesh.triangle.indices.allclose(expected_ind))
    print(f"DEBUG TC: {mesh.triangle.colors.numpy()}")
    assert(mesh.triangle.colors.allclose(expected_tri_colors))


def verify_rem_dup_success(unique_v: List[List], unique_i: List[List], dup_v: List[List], dup_i: List[List]):
    """Verifies that the remove duplicate operation on dup_v, dup_i,    
    was successful"""
    ver2uniqindx = set()
    # Make sure no duplicates are present
    for i, v in enumerate(unique_v):
        vs = str(v)
        if vs in ver2uniqindx:
            return False
        ver2uniqindx.add(vs)

    # Verify that the length of the indices did not change.
    if len(dup_i) != len(unique_i):
        return False

    # Make sure all dup and uniq indices point to
    # same vertex coordinates.
    for i, _ in enumerate(unique_i):
        for t in range(3):
            dI = dup_i[i][t]
            uI = unique_i[i][t]
            if not np.allclose(unique_v[uI], dup_v[dI]):
                return False
    return True


def gen_box_mesh_dup_vertex(unique_v, unique_i, v_index, insert_pos):
    """
    Generates a duplicate vertex list and an updated index list from
    the uniques afte inserting a vertex at v_index in unique_v to position
    insert_pos.
    """
    n_uniq = len(unique_v)
    assert (v_index < n_uniq and insert_pos <= n_uniq)
    index_old_to_new = [i for i in range(n_uniq)]
    dup_v = unique_v[:insert_pos]
    dup_v.append(unique_v[v_index])
    dup_v.extend(unique_v[insert_pos:])
    for i in range(insert_pos, n_uniq):
        index_old_to_new[i] = i + 1
    dup_i = deepcopy(unique_i)
    for triple in dup_i:
        for t in range(3):
            triple[t] = index_old_to_new[triple[t]]
    return dup_v, dup_i


def gen_rem_dup_test_cases():
    """
    Utility to generate test cases.
    The duplicates are generated at various positions.
    """
    unique_v = [[0., 0., 0.],
                [1., 0., 0.],
                [0., 0., 1.],
                [1., 0., 1.],
                [0., 1., 0.],
                [1., 1., 0.],
                [0., 1., 1.],
                [1., 1., 1.]]
    unique_i = [[4, 7, 5],
                [4, 6, 7],
                [0, 2, 4],
                [2, 6, 4],
                [0, 1, 2],
                [1, 3, 2],
                [1, 5, 7],
                [1, 7, 3],
                [2, 3, 7],
                [2, 7, 6],
                [0, 4, 1],
                [1, 4, 5]]
    nv = len(unique_v)
    # Test cases depending on position of replication.
    vertind_repind = []
    # Scenario 1: Replicate vertex at the beginning to end and middle.
    vertind_repind.extend([(0, 0), (0, nv - 1), (0, nv), (0, nv//2)])
    # Scenario 2: Replicate vertex at the end to begining and middle.
    vertind_repind.extend([(nv - 1, nv - 1), (nv - 1, 0), (nv - 1, 1), (nv - 1, nv//2)])
    # Scenario 3: Replicate vertex at 3 to ends and somewhere in the middle.
    vertind_repind.extend([(3, 0), (3, nv - 1), (3, nv//2)])
    for vind, repind in vertind_repind:
        dup_v, dup_i = gen_box_mesh_dup_vertex(unique_v, unique_i, vind, repind)
        yield dup_v, dup_i
        # print(f"Duplicates for vertex at index: {vind}, replicated at: {repind}")
        # print(f"Dup V: \n{dup_v}")
        # print(f"Dup I: \n{dup_i}")
        # Scenario 4: some double replication cases.
        if vind < repind:
            dup_v2, dup_i2 = gen_box_mesh_dup_vertex(dup_v, dup_i, vind, repind)
            yield dup_v2, dup_i2
            # print(f"Duplicates2 for vertex at index: {vind}, replicated at: {repind}")
            # print(f"Dup2 V: \n{dup_v2}")
            # print(f"Dup2 I: \n{dup_i2}")


@pytest.mark.parametrize("device", list_devices())
def test_remove_duplicate_vertices_gen6(device):
    # Runs each generated test case verifies that duplicate removal was successful.
    mesh = o3d.t.geometry.TriangleMesh(device)
    for verts, indices in gen_rem_dup_test_cases():
        mesh.vertex.positions = o3c.Tensor(verts, dtype=o3c.float64)
        mesh.triangle.indices = o3c.Tensor(indices, dtype=o3c.int32)
        mesh.remove_duplicate_vertices()
        unique_v = mesh.vertex.positions.numpy().tolist()
        unique_i = mesh.triangle.indices.numpy().tolist()
        # Verify duplicate removal was successful
        assert(verify_rem_dup_success(unique_v, unique_i, verts, indices))


@pytest.mark.parametrize("device", list_devices())
def test_pickle(device):
    mesh = o3d.t.geometry.TriangleMesh.create_box().to(device)
    with tempfile.TemporaryDirectory() as temp_dir:
        file_name = f"{temp_dir}/mesh.pkl"
        pickle.dump(mesh, open(file_name, "wb"))
        mesh_load = pickle.load(open(file_name, "rb"))
        assert mesh_load.device == device
        assert mesh_load.vertex.positions.dtype == o3c.float32
        assert mesh_load.triangle.indices.dtype == o3c.int64
        np.testing.assert_equal(mesh_load.vertex.positions.cpu().numpy(),
                                mesh.vertex.positions.cpu().numpy())
        np.testing.assert_equal(mesh_load.triangle.indices.cpu().numpy(),
                                mesh.triangle.indices.cpu().numpy())


@pytest.mark.parametrize("device", list_devices())
def test_select_faces_by_mask_32(device):
    sphere_custom = o3d.t.geometry.TriangleMesh.create_sphere(
        1, 3, o3c.float64, o3c.int32, device)

    expected_verts = o3c.Tensor(
        [[0.0, 0.0, 1.0], [0.866025, 0, 0.5], [0.433013, 0.75, 0.5],
         [-0.866025, 0.0, 0.5], [-0.433013, -0.75, 0.5], [0.433013, -0.75, 0.5]
        ], o3c.float64, device)

    expected_tris = o3c.Tensor([[0, 1, 2], [0, 3, 4], [0, 4, 5], [0, 5, 1]],
                               o3c.int32, device)

    # check indices shape mismatch
    mask_2d = o3c.Tensor([[False, False], [False, False], [False, False]],
                         o3c.bool, device)
    with pytest.raises(RuntimeError):
        selected = sphere_custom.select_faces_by_mask(mask_2d)

    # check indices type mismatch
    mask_float = o3c.Tensor([
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    ], o3c.float32, device)
    with pytest.raises(RuntimeError):
        selected = sphere_custom.select_faces_by_mask(mask_float)

    # check the basic case
    mask = o3c.Tensor([
        True, False, False, False, False, False, True, False, True, False, True,
        False, False, False, False, False, False, False, False, False, False,
        False, False, False
    ], o3c.bool, device)
    selected = sphere_custom.select_faces_by_mask(mask)
    assert selected.vertex.positions.allclose(expected_verts)
    assert selected.triangle.indices.allclose(expected_tris)

    # check that the original mesh is unmodified
    untouched_sphere = o3d.t.geometry.TriangleMesh.create_sphere(
        1, 3, o3c.float64, o3c.int32, device)
    assert sphere_custom.vertex.positions.allclose(
        untouched_sphere.vertex.positions)
    assert sphere_custom.triangle.indices.allclose(
        untouched_sphere.triangle.indices)


@pytest.mark.parametrize("device", list_devices())
def test_select_faces_by_mask_64(device):
    sphere_custom = o3d.t.geometry.TriangleMesh.create_sphere(
        1, 3, o3c.float64, o3c.int64, device)

    # check indices shape mismatch
    mask_2d = o3c.Tensor([[False, False], [False, False], [False, False]],
                         o3c.bool, device)
    with pytest.raises(RuntimeError):
        selected = sphere_custom.select_faces_by_mask(mask_2d)

    # check indices type mismatch
    mask_float = o3c.Tensor([
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    ], o3c.float32, device)
    with pytest.raises(RuntimeError):
        selected = sphere_custom.select_faces_by_mask(mask_float)

    expected_verts = o3c.Tensor(
        [[0.0, 0.0, 1.0], [0.866025, 0, 0.5], [0.433013, 0.75, 0.5],
         [-0.866025, 0.0, 0.5], [-0.433013, -0.75, 0.5], [0.433013, -0.75, 0.5]
        ], o3c.float64, device)

    expected_tris = o3c.Tensor([[0, 1, 2], [0, 3, 4], [0, 4, 5], [0, 5, 1]],
                               o3c.int64, device)
    # check the basic case
    mask = o3c.Tensor([
        True, False, False, False, False, False, True, False, True, False, True,
        False, False, False, False, False, False, False, False, False, False,
        False, False, False
    ], o3c.bool, device)

    selected = sphere_custom.select_faces_by_mask(mask)
    assert selected.vertex.positions.allclose(expected_verts)
    assert selected.triangle.indices.allclose(expected_tris)

    # check that the original mesh is unmodified
    untouched_sphere = o3d.t.geometry.TriangleMesh.create_sphere(
        1, 3, o3c.float64, o3c.int64, device)
    assert sphere_custom.vertex.positions.allclose(
        untouched_sphere.vertex.positions)
    assert sphere_custom.triangle.indices.allclose(
        untouched_sphere.triangle.indices)


@pytest.mark.parametrize("device", list_devices())
def test_select_by_index_32(device):
    sphere_custom = o3d.t.geometry.TriangleMesh.create_sphere(
        1, 3, o3c.float64, o3c.int32, device)

    expected_verts = o3c.Tensor(
        [[0.0, 0.0, 1.0], [0.866025, 0, 0.5], [0.433013, 0.75, 0.5],
         [-0.866025, 0.0, 0.5], [-0.433013, -0.75, 0.5], [0.433013, -0.75, 0.5]
        ], o3c.float64, device)

    expected_tris = o3c.Tensor([[0, 1, 2], [0, 3, 4], [0, 4, 5], [0, 5, 1]],
                               o3c.int32, device)

    # check indices shape mismatch
    indices_2d = o3c.Tensor([[0, 2], [3, 5], [6, 7]], o3c.int32, device)
    with pytest.raises(RuntimeError):
        selected = sphere_custom.select_by_index(indices_2d)

    # check indices type mismatch
    indices_float = o3c.Tensor([2.0, 4.0], o3c.float32, device)
    with pytest.raises(RuntimeError):
        selected = sphere_custom.select_by_index(indices_float)

    # check the expected mesh with int8 input
    indices_8 = o3c.Tensor([0, 2, 3, 5, 6, 7], o3c.int8, device)
    selected = sphere_custom.select_by_index(indices_8)
    assert selected.vertex.positions.allclose(expected_verts)
    assert selected.triangle.indices.allclose(expected_tris)

    # check the expected mesh with int16 input
    indices_16 = o3c.Tensor([2, 0, 5, 3, 7, 6], o3c.int16, device)
    selected = sphere_custom.select_by_index(indices_16)
    assert selected.vertex.positions.allclose(expected_verts)
    assert selected.triangle.indices.allclose(expected_tris)

    # check the expected mesh with uint32 input
    indices_u32 = o3c.Tensor([7, 6, 5, 3, 2, 0], o3c.uint32, device)
    selected = sphere_custom.select_by_index(indices_u32)
    assert selected.vertex.positions.allclose(expected_verts)
    assert selected.triangle.indices.allclose(expected_tris)

    # check the expected mesh with uint64 input and unsorted indices
    indices_u64 = o3c.Tensor([7, 6, 3, 5, 0, 2], o3c.uint64, device)
    selected = sphere_custom.select_by_index(indices_u64)
    assert selected.vertex.positions.allclose(expected_verts)
    assert selected.triangle.indices.allclose(expected_tris)

    # check that an index exceeding the max vertex index of the mesh is ignored
    selected = sphere_custom.select_by_index([0, 2, 3, 5, 6, 99, 7])
    assert selected.vertex.positions.allclose(expected_verts)
    assert selected.triangle.indices.allclose(expected_tris)

    # check that a negative index is ignored
    selected = sphere_custom.select_by_index([0, 2, 3, 5, -10, 6, 7])
    assert selected.vertex.positions.allclose(expected_verts)
    assert selected.triangle.indices.allclose(expected_tris)

    # check that the original mesh is unmodified
    untouched_sphere = o3d.t.geometry.TriangleMesh.create_sphere(
        1, 3, o3c.float64, o3c.int32, device)
    assert sphere_custom.vertex.positions.allclose(
        untouched_sphere.vertex.positions)
    assert sphere_custom.triangle.indices.allclose(
        untouched_sphere.triangle.indices)


@pytest.mark.parametrize("device", list_devices())
def test_select_by_index_64(device):
    sphere_custom = o3d.t.geometry.TriangleMesh.create_sphere(
        1, 3, o3c.float64, o3c.int64, device)

    # check indices shape mismatch
    with pytest.raises(RuntimeError):
        indices_2d = o3c.Tensor([[0, 2], [3, 5], [6, 7]], o3c.int64, device)
        selected = sphere_custom.select_by_index(indices_2d)

    # check indices type mismatch
    with pytest.raises(RuntimeError):
        indices_float = o3c.Tensor([2.0, 4.0], o3c.float64, device)
        selected = sphere_custom.select_by_index(indices_float)

    expected_verts = o3c.Tensor(
        [[0.0, 0.0, 1.0], [0.866025, 0, 0.5], [0.433013, 0.75, 0.5],
         [-0.866025, 0.0, 0.5], [-0.433013, -0.75, 0.5], [0.433013, -0.75, 0.5]
        ], o3c.float64, device)

    expected_tris = o3c.Tensor([[0, 1, 2], [0, 3, 4], [0, 4, 5], [0, 5, 1]],
                               o3c.int64, device)

    # check the expected mesh with int8 input
    indices_u8 = o3c.Tensor([0, 2, 3, 5, 6, 7], o3c.uint8, device)
    selected = sphere_custom.select_by_index(indices_u8)
    assert selected.vertex.positions.allclose(expected_verts)
    assert selected.triangle.indices.allclose(expected_tris)

    # check the expected mesh with int16 input
    indices_u16 = o3c.Tensor([2, 0, 5, 3, 7, 6], o3c.uint16, device)
    selected = sphere_custom.select_by_index(indices_u16)
    assert selected.vertex.positions.allclose(expected_verts)
    assert selected.triangle.indices.allclose(expected_tris)

    # check the expected mesh with int32 input
    indices_32 = o3c.Tensor([7, 6, 5, 3, 2, 0], o3c.int32, device)
    selected = sphere_custom.select_by_index(indices_32)
    assert selected.vertex.positions.allclose(expected_verts)
    assert selected.triangle.indices.allclose(expected_tris)

    # check the expected mesh with int64 input and unsorted indices
    indices_64 = o3c.Tensor([7, 6, 3, 5, 0, 2], o3c.int64, device)
    selected = sphere_custom.select_by_index(indices_64)
    assert selected.vertex.positions.allclose(expected_verts)
    assert selected.triangle.indices.allclose(expected_tris)

    # check that an index exceeding the max vertex index of the mesh is ignored
    selected = sphere_custom.select_by_index([0, 2, 3, 5, 6, 99, 7])
    assert selected.vertex.positions.allclose(expected_verts)
    assert selected.triangle.indices.allclose(expected_tris)

    # check that a negative index is ignored
    selected = sphere_custom.select_by_index([0, 2, 3, 5, -10, 6, 7])
    assert selected.vertex.positions.allclose(expected_verts)
    assert selected.triangle.indices.allclose(expected_tris)

    # check that the original mesh is unmodified
    untouched_sphere = o3d.t.geometry.TriangleMesh.create_sphere(
        1, 3, o3c.float64, o3c.int64, device)
    assert sphere_custom.vertex.positions.allclose(
        untouched_sphere.vertex.positions)
    assert sphere_custom.triangle.indices.allclose(
        untouched_sphere.triangle.indices)
