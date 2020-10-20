/*
 ============================================================================
 Name        : ringbuffer_3.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include "ringbuffer_3.h"

VIDEOENCBUFFERMANAGER *buffer_manager;

void pushframe(){
    FRAMEDATATYPE frame;
    while(1){
//        frame.info.timeStamp = pEncFrame->VFrame.mpts;
//        frame.info.bufferId = pEncFrame->mId;
//        frame.info.size = pEncFrame->VFrame.mWidth;
//        frame.addrY = (char*)pEncFrame->VFrame.mpVirAddr[0];
        VideoEncBufferPushFrame(buffer_manager, &frame);
    }
}

int main(void) {
      //used for managing usb camera frame, for compressed video frame such as mjpeg.
    FRAMEDATATYPE frame;
    pthread_t tidAi = -1;
    int ret = 0;
    if (buffer_manager == NULL)
    {
        buffer_manager = VideoEncBufferInit();
    }
    ret = pthread_create(&tidAi, NULL, pushframe, NULL);
    if(0 != ret)
    {
        printf("Fail to call pthread_create(3) with ()\n");
    }

    while(1){
         VideoEncBufferGetFrame(buffer_manager, &frame);

         VideoEncBufferReleaseFrame(buffer_manager, &frame);
    }

    puts("!!!Hello World!!!"); /* prints !!!Hello World!!! */
	return EXIT_SUCCESS;
}
