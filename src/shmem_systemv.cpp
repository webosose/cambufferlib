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

#include "shmem_systemv.h"
#include "camshm.h"

namespace camera {

bool SystemvSharedMemory::Open(key_t shmemKey) {
    if (phShmem_)
        return true;

    if (OpenShmem(&phShmem_, shmemKey) != SHMEM_COMM_OK) {
        phShmem_ = nullptr;
        return false;
    }

    shmKey_ = shmemKey;
    return true;
}

bool SystemvSharedMemory::Create(key_t* shmemKey, const int unitSize, const int units) {
    if (shmemKey == nullptr)
        return false;
    if (phShmem_)
        return true;

    if (CreateShmem(&phShmem_, &shmKey_, unitSize, units) != SHMEM_COMM_OK)
        return false;

    *shmemKey = shmKey_;
    return true;
}

bool SystemvSharedMemory::Close() {
    int status = SHMEM_COMM_OK;
    if (phShmem_)
        status = CloseShmem(&phShmem_);

    if (status != SHMEM_COMM_OK)
        return false;

    phShmem_ = nullptr;

    return true;
}

bool SystemvSharedMemory::ReadData(uint8_t** buffer, int* len) {
    if (!buffer || !len)
        return false;

    if (ReadShmem(phShmem_, buffer, len) != SHMEM_COMM_OK)
        return false;
    return true;
}

bool SystemvSharedMemory::WriteData(uint8_t* buffer, size_t len) {
    int status = SHMEM_COMM_FAIL;
    if (phShmem_)
        status = WriteShmem(phShmem_, buffer, len);
    return status == SHMEM_COMM_OK;
}

} // namespace camera
