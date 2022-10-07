// Copyright (c) 2022 LG Electronics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

#ifndef WEBOS_CAMERA_BUFFER_LIB_INTERFACE_H_
#define WEBOS_CAMERA_BUFFER_LIB_INTERFACE_H_

#include <inttypes.h>
#include <sys/shm.h>

namespace camera {

class ISharedMemory;

class CameraBuffer {
public:
    enum InterfaceType {
        SHMEM_POSIX,
        SHMEM_SYSTEMV
    };

    CameraBuffer(InterfaceType type);
    virtual ~CameraBuffer();

    bool Open(key_t shmemKey);
    bool Create(key_t* shmemKey, const int unitSize, const int units);
    bool Close();
    bool ReadData(uint8_t** buffer, int* len);
    bool WriteData(uint8_t* buffer, const size_t len);

private:
    CameraBuffer(const CameraBuffer&) = delete;
    CameraBuffer& operator=(const CameraBuffer&) = delete;

    ISharedMemory* shared_memory_ = nullptr;
    bool isInitialized_ = false;
};

} // namespace camera

#endif
