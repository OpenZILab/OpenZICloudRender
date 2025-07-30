# Copyright (c) @ 2022-2025 OpenZI 数化软件, All rights reserved.
#
# Licensed under GNU AFFERO GENERAL PUBLIC LICENSE VERSION 3, (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    https://www.gnu.org/licenses/agpl-3.0.en.html#license-text
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.


cmake_minimum_required(VERSION 3.20)
project(mlcrs_d3d12rhi VERSION 0.1.0)



# Select C++17 as the standard for C++ projects.
set(CMAKE_CXX_STANDARD 17)
# If C++17 is not available, downgrading to an earlier standard is NOT OK.
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Do not enable compiler specific language extensions.
set(CMAKE_CXX_EXTENSIONS OFF)

set(INC
    .
    Private
    Public
    ../Config/Public
    ../Core/Public
    ../RHI/Public
    ${MLCRS_THIRDPARTY_PATH}/NvEncoder/Public
)

set(SRC
    Private/D3D12DynamicRHI.cpp
    Private/D3D12Resources.cpp

    Public/D3D12DynamicRHI.h
    Public/D3D12Resources.h
)

set(LIB
    OpenZICore
    mlcrs_config
    mlcrs_rhi
)

mlcrs_add_lib(mlcrs_d3d12rhi "${SRC}" "${INC}" "${INC_SYS}" "${LIB}")