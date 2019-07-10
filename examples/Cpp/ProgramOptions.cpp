// ----------------------------------------------------------------------------
// -                        Open3D: www.open3d.org                            -
// ----------------------------------------------------------------------------
// The MIT License (MIT)
//
// Copyright (c) 2018 www.open3d.org
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

#include "Open3D/Open3D.h"

void PrintHelp() {
    using namespace open3d;
    PrintOpen3DVersion();
    // clang-format off
    utility::NewPrintInfo("Usage :\n");
    utility::NewPrintInfo("    > ProgramOptions [--help] [--switch] [--int i] [--double d] [--string str] [--vector (x,y,z,...)]\n");
    // clang-format on
}

int main(int argc, char *argv[]) {
    using namespace open3d;
    if (argc == 1 || utility::ProgramOptionExists(argc, argv, "--help")) {
        PrintHelp();
        return 1;
    }

    utility::NewPrintInfo("Switch is {}.\n",
                       utility::ProgramOptionExists(argc, argv, "--switch")
                               ? "ON"
                               : "OFF");
    utility::NewPrintInfo("Int is {:d}\n",
                       utility::GetProgramOptionAsInt(argc, argv, "--int"));
    utility::NewPrintInfo("Double is {:.10f}\n", utility::GetProgramOptionAsDouble(
                                                    argc, argv, "--double"));
    utility::NewPrintInfo(
            "String is {}\n",
            utility::GetProgramOptionAsString(argc, argv, "--string"));
    std::vector<std::string> strs;
    utility::SplitString(
            strs, utility::GetProgramOptionAsString(argc, argv, "--string"),
            ",.", true);
    for (auto &str : strs) {
        utility::NewPrintInfo("\tSubstring : {}\n", str);
    }
    Eigen::VectorXd vec =
            utility::GetProgramOptionAsEigenVectorXd(argc, argv, "--vector");
    utility::NewPrintInfo("Vector is (");
    for (auto i = 0; i < vec.size(); i++) {
        if (i == 0) {
            utility::NewPrintInfo("{:.2f}", vec(i));
        } else {
            utility::NewPrintInfo("{:.2f}", vec(i));
        }
    }
    utility::NewPrintInfo(")\n");
    return 0;
}
