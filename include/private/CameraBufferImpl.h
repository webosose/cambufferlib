// Copyright (c) 2024 LG Electronics, Inc.
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

#ifndef WEBOS_CAMERA_BUFFER_LIB_IMPL_H_
#define WEBOS_CAMERA_BUFFER_LIB_IMPL_H_

#include <inttypes.h>
#include <memory>

class CameraSharedMemory;
class LunaClient;

namespace camera
{

class CameraBufferImpl
{
public:
    CameraBufferImpl(const std::string &identifier);
    virtual ~CameraBufferImpl();

    bool Open(int handle);
    bool Close();
    bool ReadData(uint8_t **buffer, size_t *len);

private:
    std::unique_ptr<CameraSharedMemory> shared_memory_{nullptr};
    std::unique_ptr<LunaClient> luna_client_{nullptr};

    bool getFd(const std::string &param, std::string &resp, int &fd);
};

} // namespace camera

#endif
