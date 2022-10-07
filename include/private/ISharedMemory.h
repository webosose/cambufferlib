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

#ifndef WEBOS_SHARED_MEMEORY_INTERFACE_H_
#define WEBOS_SHARED_MEMEORY_INTERFACE_H_

#include <stddef.h>
#include <stdint.h>
#include <sys/shm.h>
#include <string>

namespace camera {

typedef void* SHMEM_HANDLE;

class ISharedMemory {
public:
    ISharedMemory() = default;
    virtual ~ISharedMemory() = default;

    virtual bool Open(key_t shmemKey) = 0;
    virtual bool Create(key_t* shmemKey, const int unitSize, const int units) = 0;
    virtual bool Close() = 0;
    virtual bool ReadData(uint8_t** buffer, int* len) = 0;
    virtual bool WriteData(uint8_t* buffer, const size_t len) = 0;

protected:
    SHMEM_HANDLE phShmem_ = nullptr;
    key_t shmKey_;
    int unitSize_ = 0;
    int units_ = 0;
    int extraSize_ = 0;
    std::string name_;
};

} // namespace camera

#endif
