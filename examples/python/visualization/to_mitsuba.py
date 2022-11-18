# ----------------------------------------------------------------------------
# -                        Open3D: www.open3d.org                            -
# ----------------------------------------------------------------------------
# The MIT License (MIT)
#
# Copyright (c) 2018-2021 www.open3d.org
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
# IN THE SOFTWARE.
# ----------------------------------------------------------------------------

import open3d as o3d
import mitsuba as mi


def render_mesh(mesh, mesh_center):
    scene = mi.load_dict({
        "type": "scene",
        "integrator": {
            "type": "path"
        },
        "light": {
            "type": "envmap",
            "filename": "/home/renes/Downloads/belfast_sunset_puresky_4k.exr",
            "scale": 1.2,
        },
        "sensor": {
            "type":
                "perspective",
            "focal_length":
                "50mm",
            "to_world":
                mi.ScalarTransform4f.look_at(origin=[0, 0, 5],
                                             target=mesh_center,
                                             up=[0, 1, 0]),
            "thefilm": {
                "type": "hdrfilm",
                "width": 1024,
                "height": 768,
            },
            "thesampler": {
                "type": "multijitter",
                "sample_count": 64,
            },
        },
        "themesh": mesh,
    })

    img = mi.render(scene, spp=256)
    return img


# Default to LLVM variant which should be available on all
# platforms. If you have a system with a CUDA device then comment out LLVM
# variant and uncomment cuda variant
mi.set_variant('llvm_ad_rgb')
# mi.set_variant('cuda_ad_rgb')

# Load mesh using Open3D
dataset = o3d.data.MonkeyModel()
mesh = o3d.t.io.read_triangle_mesh(dataset.path)
# mesh = o3d.t.io.read_triangle_mesh('/home/renes/development/intel_work/sample_data/antman_color.ply')
mesh_center = mesh.get_axis_aligned_bounding_box().get_center()
mesh.material.set_default_properties()
mesh.material.vector_properties['base_color'] = [1.0, 1.0, 1.0, 1.0]
mesh.material.texture_maps['albedo'] = o3d.t.io.read_image(
    dataset.path_map['albedo'])
mesh.material.texture_maps['roughness'] = o3d.t.io.read_image(
    dataset.path_map['roughness'])
mesh.material.texture_maps['metallic'] = o3d.t.io.read_image(
    dataset.path_map['metallic'])

print(
    'Rendering mesh with its material properties converted to Mitsuba principled BSDF'
)
mi_mesh = o3d.visualization.to_mitsuba('monkey', mesh)
img = render_mesh(mi_mesh, mesh_center.numpy())
mi.Bitmap(img).write('test.exr')

print('Rendering mesh with Mitsuba smooth plastic BSDF')
bsdf_smooth_plastic = mi.load_dict({
    'type': 'plastic',
    'diffuse_reflectance': {
        'type': 'rgb',
        'value': [0.1, 0.27, 0.36]
    },
    'int_ior': 1.9
})
mi_mesh = o3d.visualization.to_mitsuba('monkey', mesh, bsdf=bsdf_smooth_plastic)
img = render_mesh(mi_mesh, mesh_center.numpy())
mi.Bitmap(img).write('test2.exr')
