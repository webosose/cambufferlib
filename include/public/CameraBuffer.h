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
#include <memory>

namespace camera
{

class CameraBufferImpl;

class CameraBuffer
{
public:
    CameraBuffer(const std::string &identifier);
    virtual ~CameraBuffer();

    bool Open(int handle);
    bool Close();
    bool ReadData(uint8_t **buffer, size_t *len);

private:
    CameraBuffer(const CameraBuffer &)            = delete;
    CameraBuffer &operator=(const CameraBuffer &) = delete;

    std::unique_ptr<CameraBufferImpl> camera_buffer_impl_{nullptr};
};

} // namespace camera

#endif
