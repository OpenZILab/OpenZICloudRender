
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

project(
    mlcrs_util
    LANGUAGES C CXX
)

set(INC
    .
    Private
    Public
    ../Core/Public
)

set(INC_SYS

)

set(SRC
    Private/DataConverter.cpp
    Private/SystemTime.cpp

    Public/DataConverter.h
    Public/SystemTime.h
    Public/PicoJson.h
)

set(LIB
    OpenZICore
)
mlcrs_add_lib(${PROJECT_NAME} "${SRC}" "${INC}" "${INC_SYS}" "${LIB}")