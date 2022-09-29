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

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <iostream>
#include <poll.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/signalfd.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <signal.h>
#include <assert.h>
#include <sys/un.h>
#include <unistd.h>
#include <assert.h>

#include "cam_posixshm.h"

#ifdef SHMEM_COMM_DEBUG
#define DEBUG_PRINT(fmt, args...) printf("\x1b[1;40;32m[SHM_API:%s] " fmt "\x1b[0m\r\n", __FUNCTION__, ##args)
#else
#define DEBUG_PRINT(fmt, args...)
#endif

// constants

#define SHMEM_HEADER_SIZE (5 * sizeof(int))
#define SHMEM_LENGTH_SIZE sizeof(int)

enum
{
    MODE_OPEN,
    MODE_CREATE
};

enum
{
    READ_FIRST,
    READ_LAST
};

// structure define

typedef enum _SHMEM_MARK_T
{
    SHMEM_COMM_MARK_NORMAL    = 0x0,
    SHMEM_COMM_MARK_RESET     = 0x1,
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

typedef struct _SHMEM_COMM_T
{
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

SHMEM_STATUS_T _OpenPosixShmem(SHMEM_HANDLE *phShmem, int fd, int unitSize, int unitNum,
        int extraSize, int nOpenMode);
SHMEM_STATUS_T _ReadPosixShmem(SHMEM_HANDLE hShmem, unsigned char **ppData, int *pSize,
        unsigned char **ppExtraData, int *pExtraSize, int readMode);

// API functions

extern SHMEM_STATUS_T OpenPosixShmem(SHMEM_HANDLE *phShmem, int fd)
{
    return _OpenPosixShmem(phShmem, fd, 0, 0, 0, MODE_OPEN);
}

SHMEM_STATUS_T _OpenPosixShmem(SHMEM_HANDLE *phShmem, int shmfd, int unitSize, int unitNum,
        int extraSize, int nOpenMode)
{
    SHMEM_COMM_T *pShmemBuffer;
    unsigned char *pSharedmem;
    int shmemSize = 0;
    struct stat sb ;

    *phShmem = (SHMEM_HANDLE) malloc(sizeof(SHMEM_COMM_T));
    pShmemBuffer = (SHMEM_COMM_T *) *phShmem;

    if( fstat (shmfd , &sb) == -1)
    {
        DEBUG_PRINT("Failed to get size of shared memory \n");
        return SHMEM_COMM_FAIL;
    }
    shmemSize = sb.st_size;
    DEBUG_PRINT("shared memory opened successfully!\n");

    pSharedmem = (unsigned char *)mmap(0, shmemSize, PROT_READ|PROT_WRITE, MAP_SHARED, shmfd, 0);
    if(pSharedmem == MAP_FAILED)
    {
        DEBUG_PRINT("mmap failed \n");
        return SHMEM_COMM_FAIL;
    }

    pShmemBuffer->write_index = (int *) (pSharedmem);
    pShmemBuffer->read_index  = (int *) (pSharedmem + sizeof(int));
    pShmemBuffer->unit_size   = (int *) (pSharedmem + sizeof(int) * 2);
    pShmemBuffer->unit_num    = (int *) (pSharedmem + sizeof(int) * 3);
    pShmemBuffer->mark        = (SHMEM_MARK_T *) (pSharedmem + sizeof(int) * 4);
    pShmemBuffer->length_buf  = (unsigned int *) (pSharedmem + sizeof(int) * 5);

    pShmemBuffer->data_buf = pSharedmem + SHMEM_HEADER_SIZE
        + SHMEM_LENGTH_SIZE * (*pShmemBuffer->unit_num);

    // shared momory size larger than total, we use extra data
    if (shmemSize > SHMEM_HEADER_SIZE + (*pShmemBuffer->unit_size
                + SHMEM_LENGTH_SIZE) * (*pShmemBuffer->unit_num))
    {
        pShmemBuffer->extra_size = (int *) (pSharedmem + SHMEM_HEADER_SIZE
                + (*pShmemBuffer->unit_size + SHMEM_LENGTH_SIZE) * (*pShmemBuffer->unit_num));
        pShmemBuffer->extra_buf = (pSharedmem + SHMEM_HEADER_SIZE
                + (*pShmemBuffer->unit_size + SHMEM_LENGTH_SIZE) * (*pShmemBuffer->unit_num)
                + sizeof(int));
    }
    else
    {
        pShmemBuffer->extra_size = NULL;
        pShmemBuffer->extra_buf  = NULL;
    }
    *pShmemBuffer->mark = SHMEM_COMM_MARK_NORMAL;
    //Until the writter starts to write both write index and read index are
    //set to -1 . So the reader can get to know that the writter has not
    //started to write yet
    *pShmemBuffer->write_index = -1;
    *pShmemBuffer->read_index  = -1;
    DEBUG_PRINT("unitSize = %d, SHMEM_LENGTH_SIZE = %d, unit_num = %d\n",
            *pShmemBuffer->unit_size, SHMEM_LENGTH_SIZE, *pShmemBuffer->unit_num);
    DEBUG_PRINT("shared memory opened successfully!\n");
    return SHMEM_COMM_OK;
}

SHMEM_STATUS_T ReadPosixShmem(SHMEM_HANDLE hShmem, unsigned char **ppData, int *pSize)
{
    return _ReadPosixShmem(hShmem, ppData, pSize, NULL, NULL, READ_FIRST);
}

SHMEM_STATUS_T ReadLastPosixShmem(SHMEM_HANDLE hShmem, unsigned char **ppData, int *pSize)
{
    return _ReadPosixShmem(hShmem, ppData, pSize, NULL, NULL, READ_LAST);
}

SHMEM_STATUS_T ReadPosixShmemEx(SHMEM_HANDLE hShmem, unsigned char **ppData, int *pSize,
        unsigned char **ppExtraData, int *pExtraSize)
{
    return _ReadPosixShmem(hShmem, ppData, pSize, ppExtraData, pExtraSize, READ_FIRST);
}

SHMEM_STATUS_T ReadLastPosixShmemEx(SHMEM_HANDLE hShmem, unsigned char **ppData, int *pSize,
        unsigned char **ppExtraData, int *pExtraSize)
{
    return _ReadPosixShmem(hShmem, ppData, pSize, ppExtraData, pExtraSize, READ_LAST);
}

SHMEM_STATUS_T _ReadPosixShmem(SHMEM_HANDLE hShmem, unsigned char **ppData, int *pSize,
        unsigned char **ppExtraData, int *pExtraSize, int readMode)
{
    SHMEM_COMM_T *shmem_buffer = (SHMEM_COMM_T *) hShmem;
    int lread_index;
    unsigned char *read_addr;
    int size;
    static bool first_read;

    first_read = false;
    if (!shmem_buffer)
    {
        DEBUG_PRINT("shmem buffer is NULL");
        return SHMEM_COMM_FAIL;
    }
    lread_index = *shmem_buffer->write_index;

    do
    {
        if (-1 != *shmem_buffer->write_index)
        {
            if (*shmem_buffer->write_index == 0)
            {
                if (0 == first_read)
                {
                    first_read = 1;
                    continue;
                }
                else
                {
                    lread_index = *shmem_buffer->unit_num - 1;
                }
            }
            else
            {
                lread_index = *shmem_buffer->write_index - 1;
            }
            size = *(int*) (shmem_buffer->length_buf + lread_index);

            if ((size == 0) || (size > *shmem_buffer->unit_size))
            {
                DEBUG_PRINT("size error(%d)!\n", size);
                return SHMEM_COMM_FAIL;
            }

            read_addr = shmem_buffer->data_buf + (lread_index) * (*shmem_buffer->unit_size);
            *ppData = read_addr;

            *pSize = size;

            if (NULL != ppExtraData && NULL != pExtraSize)
            {
                *ppExtraData = shmem_buffer->extra_buf
                    + (lread_index) * (*shmem_buffer->extra_size);
                *pExtraSize = *shmem_buffer->extra_size;
            }
        }

        break;
    } while (1);

    return SHMEM_COMM_OK;
}

SHMEM_STATUS_T ClosePosixShmem(SHMEM_HANDLE *phShmem, \
                               const int unitSize, const int units, const int extraSize, \
                               const char* shmname, const int shmfd)
{
    void *shmem_addr;
    SHMEM_COMM_T *shmem_buffer;

    shmem_buffer = (SHMEM_COMM_T *)*phShmem;

    if (!shmem_buffer) {
      DEBUG_PRINT("shmem_bufer is NULL\n");
      return SHMEM_COMM_FAIL;
    }

    int shmemSize = SHMEM_HEADER_SIZE + (unitSize + SHMEM_LENGTH_SIZE) * units
                                           + sizeof(int) + extraSize * units;

    shmem_addr = shmem_buffer->write_index;

    if (munmap(shmem_addr, shmemSize) == -1)
        return SHMEM_COMM_FAIL;

    if (shm_unlink(shmname) == -1)
        return SHMEM_COMM_FAIL;

    close(shmfd);

    free(shmem_buffer);
    shmem_buffer = NULL;
    return SHMEM_COMM_OK;
}
