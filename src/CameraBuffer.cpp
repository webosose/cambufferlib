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

#include "CameraBuffer.h"
#include "ISharedMemory.h"
#include "shmem_posix.h"
#include "shmem_systemv.h"
#include <errno.h>
#include <unistd.h>
#include <signal.h>

namespace camera {

CameraBuffer::CameraBuffer(InterfaceType type) {
    switch (type) {
        case SHMEM_POSIX:
            shared_memory_ = new PosixSharedMemory();
         break;
        case SHMEM_SYSTEMV:
        default:
            shared_memory_ = new SystemvSharedMemory();
        break;
    }
}

CameraBuffer::~CameraBuffer() {
    if (shared_memory_) {
        delete shared_memory_;
        shared_memory_ = nullptr;
    }
}

bool CameraBuffer::Open(key_t shmemKey) {
    if (isInitialized_)
        return true;
    if (shared_memory_)
        isInitialized_ = shared_memory_->Open(shmemKey);
    return isInitialized_;
}

bool CameraBuffer::Create(key_t* shmemKey, const int unitSize, const int units) {
    if (isInitialized_)
        return true;
    if (shared_memory_)
        isInitialized_ = shared_memory_->Create(shmemKey, unitSize, units);
    return isInitialized_;
}

bool CameraBuffer::Close() {
    if (!isInitialized_)
        return false;
    bool status = false;
    if (shared_memory_)
        status = shared_memory_->Close();

    isInitialized_ = !status;
    return status;
}

bool CameraBuffer::ReadData(uint8_t** buffer, int* len) {
    if (!isInitialized_)
        return false;

    if (len == nullptr)
        return false;

    // initialize the length parameter
    *len = 0;

    int signum;
    sigset_t sigset;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGUSR1);
    sigprocmask(SIG_SETMASK, &sigset, nullptr);

    int wait_retry = 0;
    struct timespec timeout {1, 0};

    do {
        signum = sigtimedwait(&sigset, NULL, &timeout);
        if (signum >= 0) {
            int read_retry = 0;
            do {
                if (shared_memory_->ReadData(buffer, len))
                    return true;

                usleep(10000); // 10 ms delay
                read_retry++;
            } while (read_retry <= 100);
        } else if (signum == -1) {
            //timeout
            wait_retry++;
        } else {
            break;
        }
    } while (wait_retry <= 10);

    return true;
}

bool CameraBuffer::WriteData(uint8_t* buffer, const size_t len) {
    if (!isInitialized_)
        return false;
    if (shared_memory_)
        return shared_memory_->WriteData(buffer, len);
    return false;
}

} // namespace camera
