// Copyright (c) 2019-2023 LG Electronics, Inc.
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

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <climits>
#include "camshm.h"
#include "camera_log.h"

// constants

#define CAMSHKEY 7010

union semun {
  int val;
  struct semid_ds *buf;
  unsigned short *array;
  struct seminfo *__buf;
};

SHMEM_STATUS_T openShmemImpl(SHMEM_HANDLE *phShmem, key_t *pShmemKey, int unitSize, int metaSize,
                          int unitNum, int extraSize, int nOpenMode);
SHMEM_STATUS_T readShmemImpl(SHMEM_HANDLE hShmem, unsigned char **ppData, int *pSize,
                          unsigned char **ppMeta, int *pMetaSize, unsigned char **ppExtraData,
                          int *pExtraSize, int readMode);
SHMEM_STATUS_T writeShmemImpl(SHMEM_HANDLE hShmem, unsigned char *pData, int dataSize,
                           unsigned char *pMeta, int metaSize, unsigned char *pExtraData,
                           int extraDataSize);

// Internal Functions

/*shared memory protocol*/
static int lockShmem(SHMEM_COMM_T *shmem_buffer)
{
    struct sembuf sema_buffer;

    if (shmem_buffer == NULL)
    {
        PLOGE("Invalid argument");
        return -1;
    }

    sema_buffer.sem_num = 0;
    sema_buffer.sem_op = -1;
    sema_buffer.sem_flg = 0;

    return semop(shmem_buffer->sema_id, &sema_buffer, 1);
}

static int unlockShmem(SHMEM_COMM_T *shmem_buffer)
{
    struct sembuf sema_buffer;

    if (shmem_buffer == NULL)
    {
        PLOGE("Invalid argument");
        return -1;
    }

    sema_buffer.sem_num = 0;
    sema_buffer.sem_op = 1;
    sema_buffer.sem_flg = 0;

    return semop(shmem_buffer->sema_id, &sema_buffer, 1);
}

static int resetShmem(SHMEM_COMM_T *shmem_buffer)
{
    union semun se;

    if (shmem_buffer == NULL)
    {
        PLOGE("Invalid argument");
        return -1;
    }

    se.val = 0;
    return semctl(shmem_buffer->sema_id, 0, SETVAL, se);
}

int getShmemCount(SHMEM_COMM_T *shmem_buffer)
{
    if (shmem_buffer == NULL)
    {
        PLOGE("Invalid argument");
        return -1;
    }

    return semctl(shmem_buffer->sema_id, 0, GETVAL, 0);
}

int increseReadIndex(SHMEM_COMM_T *pShmem_buffer, int lread_index)
{
    if (lread_index >= INT_MAX)
        lread_index = INT_MAX;
    else
    {
        lread_index += 1;
    }
    PLOGI("lread_index = %d", lread_index);
    if (lread_index == *pShmem_buffer->unit_num)
    {
        *pShmem_buffer->read_index = 0;
        lread_index = 0;
    }
    else
        *pShmem_buffer->read_index = lread_index;

    return lread_index;
}

// API functions

SHMEM_STATUS_T CreateShmem(SHMEM_HANDLE *phShmem, key_t *pShmemKey, int unitSize, int metaSize,
                           int unitNum)
{
    return openShmemImpl(phShmem, pShmemKey, unitSize, metaSize, unitNum, 0, MODE_CREATE);
}

SHMEM_STATUS_T CreateShmemEx(SHMEM_HANDLE *phShmem, key_t *pShmemKey, int unitSize, int metaSize,
                             int unitNum, int extraSize)
{
    return openShmemImpl(phShmem, pShmemKey, unitSize, metaSize, unitNum, extraSize, MODE_CREATE);
}

extern SHMEM_STATUS_T OpenShmem(SHMEM_HANDLE *phShmem, key_t shmemKey)
{
    return openShmemImpl(phShmem, &shmemKey, 0, 0, 0, 0, MODE_OPEN);
}

SHMEM_STATUS_T openShmemImpl(SHMEM_HANDLE *phShmem, key_t *pShmemKey, int unitSize, int metaSize,
                          int unitNum, int extraSize, int nOpenMode)
{
    SHMEM_COMM_T *pShmemBuffer;
    unsigned char *pSharedmem;
    key_t shmemKey;
    int shmemSize = 0;
    int shmemMode = 0666;
    struct shmid_ds shm_stat;

    *phShmem = (SHMEM_HANDLE) calloc(1, sizeof(SHMEM_COMM_T));
    pShmemBuffer = (SHMEM_COMM_T *) *phShmem;
    if (pShmemBuffer == NULL) {
        PLOGE("pShmemBuffer is null");
        return SHMEM_COMM_FAIL;
    }

   if (nOpenMode == MODE_CREATE)
   {
        for (shmemKey = CAMSHKEY; shmemKey < 0xFFFF; shmemKey++)
        {
            pShmemBuffer->shmem_id = shmget((key_t) shmemKey, 0, 0666);
            if (pShmemBuffer->shmem_id == -1)
                break;
        }
        *pShmemKey = shmemKey;
        if (unitSize < 0 && unitNum < 0)
        {
            if (SHMEM_HEADER_SIZE + (unitSize + SHMEM_LENGTH_SIZE) * unitNum + sizeof(int) <= INT_MAX)
            {
               if (extraSize >= INT_MAX || unitNum >= INT_MAX)
                   shmemSize += 0;
               else
               {
                  if (extraSize * unitNum >= INT_MAX || shmemSize >= INT_MAX)
                      shmemSize += 0;
                  else
                  {
                      shmemSize += extraSize * unitNum;
                      shmemMode |= IPC_CREAT | IPC_EXCL;
                  }
               }

            }

        }
        else
            return SHMEM_COMM_FAIL;
    }
    else
    {
        shmemKey = *pShmemKey;
    }

    PLOGD("shmem_key=%d", shmemKey);

    pShmemBuffer->shmem_id = shmget((key_t) shmemKey, shmemSize, shmemMode);
    if (pShmemBuffer->shmem_id == -1)
    {
        PLOGE("Can't open shared memory: %s", strerror(errno));
        free(pShmemBuffer);
        return SHMEM_COMM_FAIL;
    }

    pSharedmem                = (unsigned char *) shmat(pShmemBuffer->shmem_id, NULL, 0);
    if (pSharedmem == NULL)
        return SHMEM_COMM_FAIL;
    pShmemBuffer->write_index = (int *) (pSharedmem + sizeof(int) * 0);
    pShmemBuffer->read_index  = (int *) (pSharedmem + sizeof(int) * 1);
    pShmemBuffer->unit_size   = (int *) (pSharedmem + sizeof(int) * 2);
    pShmemBuffer->meta_size   = (int *) (pSharedmem + sizeof(int) * 3);
    pShmemBuffer->unit_num    = (int *) (pSharedmem + sizeof(int) * 4);
    pShmemBuffer->mark        = (SHMEM_MARK_T *) (pSharedmem + sizeof(int) * 5);

    if (nOpenMode == MODE_OPEN || (pShmemBuffer->sema_id = semget(shmemKey, 1, shmemMode)) == -1)
    {
#ifdef SHMEM_COMM_DEBUG
        if (nOpenMode == MODE_CREATE)
        PLOGE("Failed to create semaphore : %s", strerror(errno));
#endif
        if ((pShmemBuffer->sema_id = semget((key_t) shmemKey, 1, 0666)) == -1)
        {
            PLOGE("Failed to get semaphore : %s", strerror(errno));
            free(pShmemBuffer);
            return SHMEM_COMM_FAIL;
        }
    }

    if (nOpenMode == MODE_CREATE)
    {
        *pShmemBuffer->unit_size = unitSize;
        *pShmemBuffer->meta_size = metaSize;
        *pShmemBuffer->unit_num  = unitNum;
    }
    size_t length_buf_offset = sizeof(int) * 6;

    size_t data_buf_offset = 0;
    if ((*pShmemBuffer->unit_num) >= 0)
        data_buf_offset = SHMEM_HEADER_SIZE + (SHMEM_LENGTH_SIZE) * (*pShmemBuffer->unit_num);

    size_t length_meta_offset = 0;
    if ((*pShmemBuffer->unit_size) >= 0)
    {
     if ((*pShmemBuffer->unit_size) < ULONG_MAX || SHMEM_LENGTH_SIZE < ULONG_MAX)
        {
            if (length_meta_offset < ULONG_MAX && (*pShmemBuffer->unit_size) + SHMEM_LENGTH_SIZE < ULONG_MAX)
                length_meta_offset += ((*pShmemBuffer->unit_size) + SHMEM_LENGTH_SIZE);
            if (length_meta_offset < ULONG_MAX && (*pShmemBuffer->unit_num) < ULONG_MAX)
                length_meta_offset *= (*pShmemBuffer->unit_num);
            if (length_meta_offset < ULONG_MAX && SHMEM_HEADER_SIZE < ULONG_MAX)
                length_meta_offset += SHMEM_HEADER_SIZE;

        }
    }

    size_t data_meta_offset = 0;

    if ((*pShmemBuffer->unit_size) >= ULONG_MAX || SHMEM_LENGTH_SIZE >= ULONG_MAX)
        data_meta_offset += 0;
    else{
        if (((*pShmemBuffer->unit_size) + SHMEM_LENGTH_SIZE) >= ULONG_MAX || (*pShmemBuffer->unit_num) >= ULONG_MAX)
        {
            if(((*pShmemBuffer->unit_size) + SHMEM_LENGTH_SIZE) * (*pShmemBuffer->unit_num) >= ULONG_MAX)
                data_meta_offset += 0;
            else
            {
                data_meta_offset += ((*pShmemBuffer->unit_size) + SHMEM_LENGTH_SIZE);
                data_meta_offset *= (*pShmemBuffer->unit_num);
            }
        }
    }
    if (SHMEM_LENGTH_SIZE < ULONG_MAX || (*pShmemBuffer->unit_num) < ULONG_MAX)
    {
        if (data_meta_offset < ULONG_MAX && SHMEM_LENGTH_SIZE * (*pShmemBuffer->unit_num) < ULONG_MAX)
            data_meta_offset += SHMEM_LENGTH_SIZE * (*pShmemBuffer->unit_num);
    }
    if (data_meta_offset < ULONG_MAX && SHMEM_HEADER_SIZE < ULONG_MAX)
        data_meta_offset += SHMEM_HEADER_SIZE;

    size_t extra_size_offset =
        SHMEM_HEADER_SIZE +
        ((*pShmemBuffer->unit_size) + SHMEM_LENGTH_SIZE) * (*pShmemBuffer->unit_num);
    if ((*pShmemBuffer->meta_size) > 0)
    {
        if ((*pShmemBuffer->meta_size) < ULONG_MAX && SHMEM_LENGTH_SIZE < ULONG_MAX)
        {
            if(((*pShmemBuffer->meta_size) + SHMEM_LENGTH_SIZE) < ULONG_MAX && (*pShmemBuffer->unit_num) < ULONG_MAX)
            {
                if((extra_size_offset > 0 && (*pShmemBuffer->meta_size) + SHMEM_LENGTH_SIZE) * (*pShmemBuffer->unit_num) > 0)
                    extra_size_offset += ((*pShmemBuffer->meta_size) + SHMEM_LENGTH_SIZE) * (*pShmemBuffer->unit_num);
            }
        }
    }
    if (SHMEM_HEADER_SIZE > extra_size_offset)
        return SHMEM_COMM_FAIL;
    if (*pShmemBuffer->unit_num > extra_size_offset)
        return SHMEM_COMM_FAIL;

    size_t extra_buf_offset =
        SHMEM_HEADER_SIZE +
        ((*pShmemBuffer->unit_size) + SHMEM_LENGTH_SIZE) * (*pShmemBuffer->unit_num) +
        ((*pShmemBuffer->meta_size) + SHMEM_LENGTH_SIZE) * (*pShmemBuffer->unit_num) + sizeof(int);

    pShmemBuffer->length_buf = (unsigned int *)(pSharedmem + length_buf_offset);

    pShmemBuffer->data_buf = pSharedmem + data_buf_offset;

    pShmemBuffer->length_meta = (unsigned int *)(pSharedmem + length_meta_offset);

    pShmemBuffer->data_meta = pSharedmem + data_meta_offset;

    pShmemBuffer->extra_size = NULL;
    pShmemBuffer->extra_buf  = NULL;

    if (shmctl(pShmemBuffer->shmem_id, IPC_STAT, &shm_stat) != -1)
    {
#ifdef SHMEM_COMM_DEBUG
        if (shm_stat.shm_nattch == 1)
            PLOGI("we are the first client");

#endif
        // shared memory size larger than total, we use extra data
        if (shm_stat.shm_segsz > extra_size_offset)
        {
            pShmemBuffer->extra_size = (int *)(pSharedmem + extra_size_offset);
            pShmemBuffer->extra_buf  = pSharedmem + extra_buf_offset;
        }
        else
        {
            pShmemBuffer->extra_size = NULL;
            pShmemBuffer->extra_buf  = NULL;
        }
    }

    if (nOpenMode == MODE_CREATE && pShmemBuffer->extra_size != NULL)
    {
        *pShmemBuffer->extra_size = extraSize;
    }

    *pShmemBuffer->mark = SHMEM_COMM_MARK_NORMAL;
    //Until the writter starts to write both write index and read index are
    //set to -1 . So the reader can get to know that the writter has not
    //started to write yet
    *pShmemBuffer->write_index = -1;
    *pShmemBuffer->read_index  = -1;

    resetShmem(pShmemBuffer);

    PLOGI("shared memory opened successfully!");
    return SHMEM_COMM_OK;
}

SHMEM_STATUS_T ReadShmem(SHMEM_HANDLE hShmem, unsigned char **ppData, int *pSize,
                         unsigned char **ppMeta, int *pMetaSize)
{
    return readShmemImpl(hShmem, ppData, pSize, ppMeta, pMetaSize, NULL, NULL, READ_FIRST);
}

SHMEM_STATUS_T ReadLastShmem(SHMEM_HANDLE hShmem, unsigned char **ppData, int *pSize,
                             unsigned char **ppMeta, int *pMetaSize)
{
    return readShmemImpl(hShmem, ppData, pSize, ppMeta, pMetaSize, NULL, NULL, READ_LAST);
}

SHMEM_STATUS_T ReadShmemEx(SHMEM_HANDLE hShmem, unsigned char **ppData, int *pSize,
                           unsigned char **ppMeta, int *pMetaSize, unsigned char **ppExtraData,
                           int *pExtraSize)
{
    return readShmemImpl(hShmem, ppData, pSize, ppMeta, pMetaSize, ppExtraData, pExtraSize,
                      READ_FIRST);
}

SHMEM_STATUS_T ReadLastShmemEx(SHMEM_HANDLE hShmem, unsigned char **ppData, int *pSize,
                               unsigned char **ppMeta, int *pMetaSize, unsigned char **ppExtraData,
                               int *pExtraSize)
{
    return readShmemImpl(hShmem, ppData, pSize, ppMeta, pMetaSize, ppExtraData, pExtraSize, READ_LAST);
}

SHMEM_STATUS_T readShmemImpl(SHMEM_HANDLE hShmem, unsigned char **ppData, int *pSize,
                          unsigned char **ppMeta, int *pMetaSize, unsigned char **ppExtraData,
                          int *pExtraSize, int readMode)
{
    SHMEM_COMM_T *shmem_buffer = (SHMEM_COMM_T *) hShmem;
    int lread_index;
    unsigned char *read_addr;
    int size;
    static bool first_read;

    first_read = false;
    if (!shmem_buffer)
    {
        PLOGE("shmem buffer is NULL");
        return SHMEM_COMM_FAIL;
    }
    lread_index = *shmem_buffer->write_index;

    do
    {
#ifdef SHMEM_COMM_DEBUG
        int sem_count;
        sem_count = semctl(shmem_buffer->sema_id, 0, GETVAL, 0);
        PLOGI("sem_count=%d", sem_count);
#endif
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
                    if (*shmem_buffer->unit_num > INT_MIN + 1)
                    {
                        lread_index = *shmem_buffer->unit_num - 1;
                    }
                }
            }
            else
            {
                lread_index = *shmem_buffer->write_index - 1;
            }
            size = *(int*) (shmem_buffer->length_buf + lread_index);

            if ((size == 0) || (size > *shmem_buffer->unit_size))
            {
                PLOGE("size error(%d)!", size);
                return SHMEM_COMM_SIZE;
            }

            read_addr = shmem_buffer->data_buf;
            if (lread_index >= INT_MAX || (*shmem_buffer->unit_size) >= INT_MAX)
                read_addr += INT_MAX;
            else
            {
               if (((lread_index) * (*shmem_buffer->unit_size)) >= INT_MAX)
                     read_addr += INT_MAX;
               else
                     read_addr += (lread_index) * *(shmem_buffer->unit_size);
            }
            *ppData   = read_addr;
            *pSize    = size;

            if (NULL != ppMeta && NULL != pMetaSize)
            {
                size       = *(int *)(shmem_buffer->length_meta + lread_index);
                read_addr  = shmem_buffer->data_meta;
                if (lread_index >= INT_MAX || (*shmem_buffer->meta_size) >= INT_MAX)
                    read_addr += INT_MAX;
                else
                {
                   if ((lread_index) * (*shmem_buffer->meta_size) >= INT_MAX)
                        read_addr += INT_MAX;
                   else
                       read_addr += (lread_index) * *(shmem_buffer->meta_size);
                }

                *ppMeta    = read_addr;
                *pMetaSize = size;
            }

            if (NULL != ppExtraData && NULL != pExtraSize)
            {
               *ppExtraData = shmem_buffer->extra_buf;
                if (lread_index >= INT_MAX || (*shmem_buffer->extra_size) >= INT_MAX)
                    *ppExtraData += INT_MAX;
                else
                {
                   if ((lread_index) * (*shmem_buffer->extra_size) >= INT_MAX)
                        *ppExtraData += INT_MAX;
                   else
                        *ppExtraData += (lread_index) * *(shmem_buffer->extra_size);
                }
                *pExtraSize = *shmem_buffer->extra_size;
            }
        }

        break;
    } while (1);

    return SHMEM_COMM_OK;
}

SHMEM_STATUS_T WriteShmemEx(SHMEM_HANDLE hShmem, unsigned char *pData, int dataSize,
                            unsigned char *pMeta, int metaSize, unsigned char *pExtraData,
                            int extraDataSize)
{
    return writeShmemImpl(hShmem, pData, dataSize, pMeta, metaSize, pExtraData, extraDataSize);
}

SHMEM_STATUS_T WriteShmem(SHMEM_HANDLE hShmem, unsigned char *pData, int dataSize,
                          unsigned char *pMeta, int metaSize)
{
    return writeShmemImpl(hShmem, pData, dataSize, pMeta, metaSize, NULL, 0);
}

SHMEM_STATUS_T writeShmemImpl(SHMEM_HANDLE hShmem, unsigned char *pData, int dataSize,
                           unsigned char *pMeta, int metaSize, unsigned char *pExtraData,
                           int extraDataSize)
{
    SHMEM_COMM_T *shmem_buffer = (SHMEM_COMM_T *) hShmem;
    int lread_index;
    int lwrite_index;
    int mark;
    int unit_size;
    int meta_size;
    int unit_num;

    if (!shmem_buffer)
    {
        PLOGE("shmem_buffer is NULL");
        return SHMEM_COMM_FAIL;
    }

    if (-1 == *shmem_buffer->write_index)
    {
        *shmem_buffer->write_index = 0;
    }
#ifdef SHMEM_COMM_DEBUG
    {
        int sem_count;
        sem_count = semctl(shmem_buffer->sema_id, 0, GETVAL, 0);
    }
#endif

    mark         = *shmem_buffer->mark;
    unit_size    = *shmem_buffer->unit_size;
    meta_size    = *shmem_buffer->meta_size;
    unit_num     = *shmem_buffer->unit_num;
    lwrite_index = *shmem_buffer->write_index;
    if (extraDataSize > 0 && extraDataSize != *shmem_buffer->extra_size)
    {
        PLOGE("extraDataSize should be same with extrasize used when open");
        return SHMEM_COMM_FAIL;
    }

    if (mark == SHMEM_COMM_MARK_RESET)
    {
        PLOGW("warning - read process isn't reset yet!");
    }

    if ((dataSize == 0) || (dataSize > unit_size))
    {
        PLOGE("size error(%d > %d)!", dataSize, unit_size);
        return SHMEM_COMM_FAIL;
    }

    //Once the writer writes the last buffer, it is made to point to the first
    //buffer again
    if (unit_num < INT_MAX && unit_num - 1 < INT_MAX)
    {
       if (lwrite_index == (unit_num - 1))
       {
           //*shmem_buffer->mark = (*shmem_buffer->mark) | SHMEM_COMM_MARK_RESET;
           *shmem_buffer->write_index = 0;
           resetShmem(shmem_buffer);
           unlockShmem(shmem_buffer);
           return SHMEM_COMM_OVERFLOW;
       }
    }
    *(int *) (shmem_buffer->length_buf + lwrite_index) = dataSize;
    if (lwrite_index >= INT_MAX || (*shmem_buffer->unit_size) >= INT_MAX)
        memcpy(shmem_buffer->data_buf + INT_MAX, pData, dataSize);
    else
    {
       if (lwrite_index * (*shmem_buffer->unit_size) >= INT_MAX)
           memcpy(shmem_buffer->data_buf + INT_MAX, pData, dataSize);
       else
           memcpy(shmem_buffer->data_buf + lwrite_index * (*shmem_buffer->unit_size), pData, dataSize);
    }
    if (metaSize < meta_size)
    {
        *(int *)(shmem_buffer->length_meta + lwrite_index) = metaSize;
        if (lwrite_index >= INT_MAX || (*shmem_buffer->meta_size) >= INT_MAX)
            memcpy(shmem_buffer->data_meta + INT_MAX, pMeta, metaSize);
        else
        {
            if (lwrite_index * (*shmem_buffer->meta_size) >= INT_MAX)
                memcpy(shmem_buffer->data_meta + INT_MAX, pMeta, metaSize);
            else
                memcpy(shmem_buffer->data_meta + lwrite_index * (*shmem_buffer->meta_size), pMeta,metaSize);
        }
    }

    if (NULL != pExtraData && extraDataSize > 0)
    {
        memcpy(shmem_buffer->extra_buf + lwrite_index * (*shmem_buffer->extra_size), pExtraData,
                extraDataSize);
    }

    *shmem_buffer->write_index += 1;
    if (*shmem_buffer->write_index == *shmem_buffer->unit_num)
        *shmem_buffer->write_index = 0;

    unlockShmem(shmem_buffer);

#ifdef SHMEM_COMM_DEBUG
    //PLOGI("Write %u bytes[RI=%d, WI=%d]",size, *shmem_buffer->read_index, *shmem_buffer->write_index);
#endif

    return SHMEM_COMM_OK;
}

SHMEM_STATUS_T CloseShmem(SHMEM_HANDLE *phShmem)
{
    void *shmem_addr;
    struct shmid_ds shm_stat;
    SHMEM_COMM_T *shmem_buffer;

    shmem_buffer = (SHMEM_COMM_T *) *phShmem;

    if (!shmem_buffer)
    {
        PLOGE("shmem_bufer is NULL");
        return SHMEM_COMM_FAIL;
    }

    shmem_addr = shmem_buffer->write_index;
    shmdt(shmem_addr);

    resetShmem(shmem_buffer);
    unlockShmem(shmem_buffer);

    if (shmctl(shmem_buffer->shmem_id, IPC_STAT, &shm_stat) != -1)
    {
        if (shm_stat.shm_nattch == 0)
        {
            PLOGI("This is the only attached client");
            semctl(shmem_buffer->sema_id, 0, IPC_RMID, NULL);
            shmctl(shmem_buffer->shmem_id, IPC_RMID, NULL);
        }
    }

    free(shmem_buffer);
    shmem_buffer = NULL;
    PLOGI("ok");
    return SHMEM_COMM_OK;
}
