# Copyright (c) 2022 LG Electronics, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 2.8.7)

project(CameraBuffer)

if (NOT DEFINED ENV{PKG_CONFIG_PATH})
  set(ENV{PKG_CONFIG_PATH} ${WEBOS_INSTALL_ROOT}usr/share/pkgconfig)
endif()

option(TEST_APP "Generates a standalone app to test the library" OFF)

find_package(PkgConfig REQUIRED)

pkg_check_modules(PMLOGLIB REQUIRED PmLogLib)
include_directories(${PMLOGLIB_INCLUDE_DIRS})

include_directories(include/public)
include_directories(include/private)

add_subdirectory(src)

if (TEST_APP)
    add_subdirectory(test)
endif ()
