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

#ifndef _UTILS_CAM_SHM_DEFINITIONS_
#define _UTILS_CAM_SHM_DEFINITIONS_

typedef enum _SHMEM_STATUS_T {
  SHMEM_COMM_OK = 0x0,
  SHMEM_COMM_FAIL = -1,
  SHMEM_COMM_OVERFLOW = -2,
  SHMEM_COMM_NODATA = -3,
  SHMEM_COMM_TERMINATE = -4,
  SHMEM_COMM_SIZE = -5,
} SHMEM_STATUS_T;

typedef void* SHMEM_HANDLE;

#ifdef SHMEM_COMM_DEBUG
#define DEBUG_PRINT(fmt, args...)                                              \
  printf("\x1b[1;40;32m[SHM_API:%s] " fmt "\x1b[0m\r\n", __FUNCTION__, ##args)
#else
#define DEBUG_PRINT(fmt, args...)
#endif

#define SHMEM_HEADER_SIZE (5 * sizeof(int))
#define SHMEM_LENGTH_SIZE sizeof(int)

enum { MODE_OPEN, MODE_CREATE };

enum { READ_FIRST, READ_LAST };

// structure define

typedef enum _SHMEM_MARK_T {
  SHMEM_COMM_MARK_NORMAL = 0x0,
  SHMEM_COMM_MARK_RESET = 0x1,
  SHMEM_COMM_MARK_TERMINATE = 0x2
} SHMEM_MARK_T;

/* shared memory structure
 4 bytes          : write_index
 4 bytes          : read_index
 4 bytes          : unit_size
 4 bytes          : unit_num
 4 bytes          : mark
 4 bytes  *unit_num : length data
 unit_size*unit_num : data
 4 bytes         : extra_size
 extra_size*unit_num : extra data
 */

typedef struct _SHMEM_COMM_T {
  int shmem_id;
  int sema_id;

  /*shared memory overhead*/
  int *write_index;
  int *read_index;
  int *unit_size;
  int *unit_num;
  SHMEM_MARK_T *mark;

  unsigned int *length_buf;
  unsigned char *data_buf;

  int *extra_size;
  unsigned char *extra_buf;
} SHMEM_COMM_T;

#endif
