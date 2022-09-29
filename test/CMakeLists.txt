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

set(SRC_LIST
    main.cpp
    )

set(BIN_NAME CamBuffStandalone)

find_package(OpenCV)

if (OpenCV_FOUND)
    add_definitions(-DENABLE_OUTPUT)
endif ()

add_executable(${BIN_NAME} ${SRC_LIST})

target_link_libraries(${BIN_NAME} PUBLIC ${PROJECT_NAME})

if (OpenCV_FOUND)
    target_include_directories(${BIN_NAME} PUBLIC ${OpenCV_INCLUDE_DIRS})
    target_link_libraries     (${BIN_NAME} PUBLIC ${OpenCV_LIBRARIES})
endif ()

install(TARGETS ${BIN_NAME} DESTINATION bin)
