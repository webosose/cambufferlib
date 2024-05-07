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

#ifndef SRC_HAL_UTILS_POCAMSHM_H_
#define SRC_HAL_UTILS_POCAMSHM_H_

#include "definitions.h"

extern SHMEM_STATUS_T OpenPosixShmem(SHMEM_HANDLE *phShmem, int fd);
extern SHMEM_STATUS_T ReadPosixShmem(SHMEM_HANDLE hShmem, unsigned char **ppData, int *pSize);
extern SHMEM_STATUS_T ReadPosixShmemEx(SHMEM_HANDLE hShmem, unsigned char **ppData, int *pSize,
                      unsigned char **ppExtraData, int *pExtraSize);
extern SHMEM_STATUS_T ClosePosixShmem(SHMEM_HANDLE *phShmem, const int unitSize, const int units,
                      const int extraSize, const char* shmname, const int shmfd);

#endif //SRC_HAL_UTILS_POCAMSHM_H_
