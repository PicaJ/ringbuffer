#include "ringbuffer_3.h"
#include <sys/types.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

VIDEOENCBUFFERMANAGER *VideoEncBufferInit(void)
{
    VIDEOENCBUFFERMANAGER *manager;

    printf("");
    manager = (VIDEOENCBUFFERMANAGER*)malloc(sizeof(VIDEOENCBUFFERMANAGER));
    if (NULL == manager)
    {
        printf("Failed to alloc VIDEOENCBUFFERMANAGER(%s)", strerror(errno));
        return NULL;
    }
    memset(manager, 0, sizeof(VIDEOENCBUFFERMANAGER));

    manager->buffer = (unsigned char*)malloc(COMPRESSED_SRC_ENC_BUF_LEN * sizeof(unsigned char));
    if (NULL == manager->buffer)
    {
        printf("Failed to alloc buffer(%s)", strerror(errno));
        free(manager);
        return NULL;
    }
    pthread_mutex_init(&manager->lock ,NULL);

    return manager;
}

void VideoEncBufferDeInit(VIDEOENCBUFFERMANAGER *manager)
{
//    printf("");
    if (NULL == manager)
    {
        return;
    }
    pthread_mutex_destroy(&manager->lock);
    free(manager->buffer);
    free(manager);
}

int VideoEncBufferPushFrame(VIDEOENCBUFFERMANAGER *manager, FRAMEDATATYPE *frame)
{
    int eError = 0;
    unsigned int length;
    unsigned int flag = FRAME_BEGIN_FLAG;
    unsigned char  *ptr, *ptr_ori;

    if (manager == NULL)
    {
        printf("Buffer manager is NULL!");
        return ERR_VENC_NULL_PTR;
    }

    length = frame->info.size + sizeof(FRAMEINFOTYPE) + sizeof(unsigned int);
    pthread_mutex_lock(&manager->lock);
    if (manager->writePos > manager->readPos)
    {
        if (manager->writePos + length <= COMPRESSED_SRC_ENC_BUF_LEN)
        {
            ptr_ori = ptr = manager->buffer + manager->writePos;
            memcpy(ptr, &flag, sizeof(unsigned int));
            ptr += sizeof(unsigned int);
            memcpy(ptr, &frame->info, sizeof(FRAMEINFOTYPE));
            ptr += sizeof(FRAMEINFOTYPE);
            memcpy(ptr, (void*)frame->addrY, frame->info.size);
            ptr += frame->info.size;
            manager->writePos += (ptr - ptr_ori);
            manager->count++;
            manager->mUnprefetchFrameNum++;
            if (manager->writePos + sizeof(unsigned int) <= COMPRESSED_SRC_ENC_BUF_LEN)
            {
                memset(ptr, 0, sizeof(unsigned int));
            }
        }
        else
        {
            if (length <= manager->readPos)
            {
                ptr_ori = ptr = manager->buffer;
                memcpy(ptr, &flag, sizeof(unsigned int));
                ptr += sizeof(unsigned int);
                memcpy(ptr, &frame->info, sizeof(FRAMEINFOTYPE));
                ptr += sizeof(FRAMEINFOTYPE);
                memcpy(ptr, (void*)frame->addrY, frame->info.size);
                ptr += frame->info.size;
                manager->writePos = (ptr - ptr_ori);
                manager->count++;
                manager->mUnprefetchFrameNum++;
            }
            else
            {
                printf("Buffer full, %d frames, writePos=%d, readPos=%d, frame_size=%d!",
                    manager->count, manager->writePos, manager->readPos, frame->info.size);
                eError = ERR_VENC_BUF_FULL;
            }
        }
    }
    else if (manager->writePos < manager->readPos)
    {
        if (manager->readPos - manager->writePos >= length)
        {
            ptr_ori = ptr = manager->buffer + manager->writePos;
            memcpy(ptr, &flag, sizeof(unsigned int));
            ptr += sizeof(unsigned int);
            memcpy(ptr, &frame->info, sizeof(FRAMEINFOTYPE));
            ptr += sizeof(FRAMEINFOTYPE);
            memcpy(ptr, (void*)frame->addrY, frame->info.size);
            ptr += frame->info.size;
            manager->writePos += (ptr - ptr_ori);
            manager->count++;
            manager->mUnprefetchFrameNum++;
        }
        else
        {
            printf("Buffer full, %d frames, writePos=%d, readPos=%d, frame_size=%d!",
                manager->count, manager->writePos, manager->readPos, frame->info.size);
            eError = ERR_VENC_BUF_FULL;
        }
    }
    else
    {
        if (manager->count == 0)
        {
            manager->readPos = 0;
            ptr_ori = ptr = manager->buffer;
            memcpy(ptr, &flag, sizeof(unsigned int));
            ptr += sizeof(unsigned int);
            memcpy(ptr, &frame->info, sizeof(FRAMEINFOTYPE));
            ptr += sizeof(FRAMEINFOTYPE);
            memcpy(ptr, (void*)frame->addrY, frame->info.size);
            ptr += frame->info.size;
            manager->writePos = (ptr - ptr_ori);
            manager->count++;
            manager->mUnprefetchFrameNum++;
        }
        else
        {
            printf("Buffer full, %d frames, writePos=%d, readPos=%d, frame_size=%d!",
                manager->count, manager->writePos, manager->readPos, frame->info.size);
            eError = ERR_VENC_BUF_FULL;
        }
    }
    pthread_mutex_unlock(&manager->lock);
    return eError;
}

int VideoEncBufferGetFrame(VIDEOENCBUFFERMANAGER *manager, FRAMEDATATYPE *frame)
{
    int eError = 0;

    if (manager == NULL)
    {
        printf("Buffer manager is NULL!");
        return ERR_VENC_ILLEGAL_PARAM;
    }

    pthread_mutex_lock(&manager->lock);
    if (manager->mUnprefetchFrameNum > 0)
    {
        unsigned int flag;
        unsigned char  *ptr;

        if (manager->prefetchPos + sizeof(unsigned int) < COMPRESSED_SRC_ENC_BUF_LEN)
        {
            memcpy(&flag, manager->buffer+manager->prefetchPos, sizeof(unsigned int));
            if (flag == FRAME_BEGIN_FLAG)
            {
                ptr = manager->buffer + manager->prefetchPos;
            }
            else
            {
                ptr = manager->buffer;
            }
        }
        else
        {
            ptr = manager->buffer;
        }
        ptr += sizeof(unsigned int);
        memcpy(&frame->info, ptr, sizeof(FRAMEINFOTYPE));
        ptr += sizeof(FRAMEINFOTYPE);
        frame->addrY = (char*)ptr;
        manager->mUnprefetchFrameNum--;
        ptr += frame->info.size;
        manager->prefetchPos = ptr - manager->buffer;
    }
    else
    {
        //printf("Buffer empty!");
        eError = ERR_VENC_BUF_EMPTY;
    }
    pthread_mutex_unlock(&manager->lock);
    return eError;
}

int VideoEncBufferReleaseFrame(VIDEOENCBUFFERMANAGER *manager, FRAMEDATATYPE *releaseFrame)
{
    FRAMEDATATYPE frame;
    int eError = 0;

    if (manager == NULL)
    {
        printf("Buffer manager is NULL!");
        return ERR_VENC_ILLEGAL_PARAM;
    }

    pthread_mutex_lock(&manager->lock);
    if (manager->count > 0)
    {
        unsigned int flag;
        if (manager->readPos + sizeof(unsigned int) < COMPRESSED_SRC_ENC_BUF_LEN)
        {
            memcpy(&flag, manager->buffer+manager->readPos, sizeof(unsigned int));
            if (flag != FRAME_BEGIN_FLAG)
            {
                manager->readPos = 0;
            }
        }
        else
        {
            manager->readPos = 0;
        }
        manager->readPos += sizeof(unsigned int);
        memcpy(&frame.info, manager->buffer + manager->readPos, sizeof(FRAMEINFOTYPE));
        if(releaseFrame->addrY!=frame.addrY
            || releaseFrame->info.timeStamp!=frame.info.timeStamp
            || releaseFrame->info.bufferId!=frame.info.bufferId
            || releaseFrame->info.size!=frame.info.size)
        {
            printf("fatal error! release frame is not match current read frame!addrY[%p]timeStamp[%lld]bufferId[%d]size[%d]!=[%p][%lld][%d][%d]",
                releaseFrame->addrY, releaseFrame->info.timeStamp, releaseFrame->info.bufferId, releaseFrame->info.size,
                frame.addrY, frame.info.timeStamp, frame.info.bufferId, frame.info.size);
        }
        manager->readPos += sizeof(FRAMEINFOTYPE) + frame.info.size;
        manager->count--;
    }
    else
    {
        //printf("Buffer empty!");
        eError = ERR_VENC_BUF_EMPTY;
    }
    pthread_mutex_unlock(&manager->lock);
    return eError;
}
