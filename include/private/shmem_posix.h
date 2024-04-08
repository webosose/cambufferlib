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

#ifndef WEBOS_SHARED_MEMEORY_POSIX_H_
#define WEBOS_SHARED_MEMEORY_POSIX_H_

#include "ISharedMemory.h"

namespace camera {

class PosixSharedMemory : public ISharedMemory {
public:
    PosixSharedMemory() = default;
    virtual ~PosixSharedMemory() { closeImpl(); }

    virtual bool Open(key_t shmemKey);
    virtual bool Create(key_t* shmemKey, const int unitSize, const int units);
    virtual bool Close() { return closeImpl(); }
    virtual bool ReadData(uint8_t** buffer, int* len);
    virtual bool WriteData(uint8_t* buffer, const size_t len);

private:
    bool closeImpl();
};

} // namespace camera

#endif
