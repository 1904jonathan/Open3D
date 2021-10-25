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

# examples/python/benchmark/benchmark_pre.py

import os
import sys
sys.path.append("../pipelines")
sys.path.append("../geometry")
sys.path.append("../utility")
import numpy as np
from file import *
from visualization import *
from downloader import *
from fast_global_registration import *
from trajectory_io import *

import pickle

do_visualization = False


def get_ply_path(dataset_name, id):
    return "%s/%s/cloud_bin_%d.ply" % (dataset_path, dataset_name, id)


def get_log_path(dataset_name):
    return "%s/fgr_%s.log" % (dataset_path, dataset_name)


if __name__ == "__main__":
    # data preparation
    redwood = o3d.data.dataset.Redwood()
    voxel_size = 0.05

    # do RANSAC based alignment
    for ply_file_list in redwood.ply_paths:

        alignment = []
        for ply_file in ply_file_list:
            source = o3d.io.read_point_cloud(ply_file)
            source_down, source_fpfh = preprocess_point_cloud(
                source, voxel_size)
            f = open('store.pckl', 'wb')
            pickle.dump([source_down, source_fpfh], f)
            f.close()
