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
#include <iostream>

#include "shmem_posix.h"
#include "cam_posixshm.h"

namespace camera {

bool PosixSharedMemory::Open(key_t fd) {
    if (phShmem_)
        return true;

    if (OpenPosixShmem(&phShmem_, fd) != SHMEM_COMM_OK)
        return false;

    shmKey_ = fd;
    return true;
}

bool PosixSharedMemory::Create(key_t* shmemKey, const int unitSize, const int units) {
    std::cerr << "posix shmem Create() is not yet implemented" << std::endl;
    return false;
}

bool PosixSharedMemory::Close() {
    int status = SHMEM_COMM_OK;
    if (phShmem_)
        status = ClosePosixShmem(&phShmem_, unitSize_, units_, extraSize_, name_.c_str(), shmKey_ /* fd */);

    if (status != SHMEM_COMM_OK)
        return false;

    phShmem_ = nullptr;

    return true;
}

bool PosixSharedMemory::ReadData(uint8_t** buffer, int* len) {
    if (!buffer || !len)
        return false;

    if (ReadPosixShmem(phShmem_, buffer, len) != SHMEM_COMM_OK)
      return false;
    return true;
}

bool PosixSharedMemory::WriteData(uint8_t* buffer, size_t len) {
    std::cerr << "posix shmem WriteData() is not yet implemented" << std::endl;
    return false;
}

} // namespace camera
