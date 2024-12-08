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

#define LOG_TAG "CameraBuffer"
#include "CameraBuffer.h"
#include "CameraBufferImpl.h"
#include "camera_log.h"

namespace camera
{

CameraBuffer::CameraBuffer(const std::string &identifier)
{
    PLOGI("");
    camera_buffer_impl_ = std::make_unique<CameraBufferImpl>(identifier);
}

CameraBuffer::~CameraBuffer() { PLOGI(""); }

bool CameraBuffer::Open(int handle)
{
    PLOGI("");
    return camera_buffer_impl_->Open(handle);
}

bool CameraBuffer::Close()
{
    PLOGI("");
    return camera_buffer_impl_->Close();
}

bool CameraBuffer::ReadData(uint8_t **buffer, size_t *len)
{
    return camera_buffer_impl_->ReadData(buffer, len);
}

} // namespace camera
