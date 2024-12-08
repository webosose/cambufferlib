# Copyright (c) 2022-2023 LG Electronics, Inc.
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

set(SRC_LIST
    CameraBuffer.cpp
    CameraBufferImpl.cpp
    )

file(GLOB PUBLIC_HEADERS ${CMAKE_SOURCE_DIR}/include/public/*.h)

add_library(${PROJECT_NAME} SHARED ${SRC_LIST})

target_link_libraries(${PROJECT_NAME}
                      rt
                      camera_shared_memory
                      luna_client)

set_target_properties(${PROJECT_NAME}
    PROPERTIES
        VERSION 1.0 SOVERSION 1
    )

set_target_properties(${PROJECT_NAME}
    PROPERTIES
        PUBLIC_HEADER "${PUBLIC_HEADERS}"
    )

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${WEBOS_INSTALL_LIBDIR})
install(TARGETS ${PROJECT_NAME} PUBLIC_HEADER DESTINATION ${WEBOS_INSTALL_INCLUDEDIR})

#install package config file
set(PC_FILE "camera-buffer")
configure_file(${PC_FILE}.pc.in ${CMAKE_BINARY_DIR}/${PC_FILE}.pc @ONLY)
install(FILES ${CMAKE_BINARY_DIR}/${PC_FILE}.pc DESTINATION share/pkgconfig)
