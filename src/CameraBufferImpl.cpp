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

#define LOG_TAG "CameraBufferImpl"
#include "CameraBufferImpl.h"
#include "camera/camera_shared_memory.h"
#include "camera/luna_client.h"
#include "camera_log.h"
#include "json_util.h"
#include <random>

namespace camera
{

#define COMMAND_TIMEOUT 3000 // ms
static const std::string luna_service_uri = "luna://com.webos.service.camera2/";

std::string generateRandomNumber()
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 9);

    std::string number;
    for (int i = 0; i < 6; ++i)
    {
        number += std::to_string(dis(gen));
    }

    return number;
}

CameraBufferImpl::CameraBufferImpl(const std::string &identifier)
{
    PLOGI("");

    shared_memory_ = std::make_unique<CameraSharedMemory>();

    std::string name = identifier;
    if (!name.empty() && *name.rbegin() != '.' && *name.rbegin() != '-')
        name += "-";

    if (!name.empty())
    {
        name += "cambuf_" + generateRandomNumber();
    }

    PLOGI("service name: %s", name.c_str());

    luna_client_ = std::make_unique<LunaClient>(name.c_str());
}

CameraBufferImpl::~CameraBufferImpl() { PLOGI(""); }

bool CameraBufferImpl::Open(int handle)
{
    PLOGI("");

    int bufferFd = -1;
    int signalFd = -1;

    std::string getFdResp;
    json jFdParam;

    jFdParam["handle"] = handle;
    jFdParam["type"]   = "buffer";
    bool ret           = getFd(jFdParam.dump(), getFdResp, bufferFd);
    if (!ret || bufferFd < 0)
    {
        PLOGE("getFd fail!");
        return false;
    }

    jFdParam["type"] = "signal";
    ret              = getFd(jFdParam.dump(), getFdResp, signalFd);
    if (!ret || signalFd < 0)
    {
        PLOGE("getFd fail!");
        return false;
    }

    return shared_memory_->open(bufferFd, signalFd);
}

bool CameraBufferImpl::Close()
{
    PLOGI("");
    shared_memory_->close();
    return true;
}

bool CameraBufferImpl::ReadData(uint8_t **buffer, size_t *len)
{
    return shared_memory_->read(buffer, len, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
}

bool CameraBufferImpl::getFd(const std::string &param, std::string &resp, int &fd)
{
    PLOGI("start! param(%s)", param.c_str());

    std::string uri = luna_service_uri + __func__;
    luna_client_->callSync(uri.c_str(), param.c_str(), &resp, COMMAND_TIMEOUT, &fd);

    PLOGI("end! resp(%s) fd(%d)", resp.c_str(), fd);

    json j = json::parse(resp);
    if (j.is_discarded())
    {
        PLOGE("resp parsing error!");
        return false;
    }

    return get_optional<bool>(j, "returnValue").value_or(false);
}

} // namespace camera
