// xiSample.cpp : program that captures 10 images

#include "stdafx.h"

#ifdef WIN32
#include "xiApi.h"       // Windows
//#include "xiExt.h"
#else
#include <m3api/xiApi.h> // Linux, OSX
#endif
#include <memory.h>
#include <time.h>	
#include <conio.h>


#define HandleResult(res,place) if (res!=XI_OK) {printf("Error after %s (%d)\n",place,res);goto finish;}

#define EXPECTED_IMAGES 100

extern "C" int xiSample()
{
    clock_t start_time, end_time, t;

    // image buffer
    XI_IMG image;
    memset(&image, 0, sizeof(image));
    image.size = sizeof(XI_IMG);

    // Sample for XIMEA API V4.05
    HANDLE xiH = NULL;
    XI_RETURN stat = XI_OK;

    // Retrieving a handle to the camera device 
    printf("Opening first camera...\n");
    stat = xiOpenDevice(0, &xiH);
    HandleResult(stat, "xiOpenDevice");

    stat = xiSetParamInt(xiH, XI_PRM_IMAGE_DATA_FORMAT, XI_RAW16);
    HandleResult(stat, "xiSetParam (XI_PRM_IMAGE_DATA_FORMAT)");


    stat = xiSetParamInt(xiH, XI_PRM_EXPOSURE, 10000);
    HandleResult(stat, "xiSetParam (XI_PRM_IMAGE_DATA_FORMAT)");

    stat = xiSetParamInt(xiH, XI_PRM_WIDTH, 3840);
    HandleResult(stat, "XI_PRM_WIDTH");
    stat = xiSetParamInt(xiH, XI_PRM_OFFSET_X, 640);
    HandleResult(stat, "XI_PRM_OFFSET_X");

    stat = xiSetParamInt(xiH, XI_PRM_ACQ_BUFFER_SIZE, 5120 * 3840 * 2 * 4);
    HandleResult(stat, "XI_PRM_ACQ_BUFFER_SIZE");

    stat = xiSetParamInt(xiH, XI_PRM_BUFFERS_QUEUE_SIZE, 2);
    HandleResult(stat, "XI_PRM_BUFFERS_QUEUE_SIZE");

    stat = xiSetParamInt(xiH, XI_PRM_RECENT_FRAME, 1);
    HandleResult(stat, "XI_PRM_RECENT_FRAME");

    stat = xiSetParamInt(xiH, XI_PRM_OUTPUT_DATA_PACKING, XI_OFF);
    HandleResult(stat, "XI_PRM_RECENT_FRAME");

    stat = xiSetParamInt(xiH, XI_PRM_LIMIT_BANDWIDTH, 5000);
    HandleResult(stat, "XI_PRM_LIMIT_BANDWIDTH");

    stat = xiStartAcquisition(xiH);
    HandleResult(stat, "xiStartAcquisition");

    start_time = clock();

    for (int images = 0; images < EXPECTED_IMAGES; images++)
    {
        // getting image from camera

        stat = xiGetImage(xiH, 5000, &image);
        HandleResult(stat, "xiGetImage");
        printf(".");

    }
    end_time = clock();

    stat = xiStopAcquisition(xiH);
    HandleResult(stat, "xiStopAcquisition");

    t = end_time - start_time;
    float seconds = (float)t / CLOCKS_PER_SEC;
    float frame_rate = (float)EXPECTED_IMAGES / seconds;
    float cycle_time_free_run = 1000 / frame_rate;

    printf("\n cycle_time_free_run = %.2f ms frame_rate is %.2f FPS\n", cycle_time_free_run, frame_rate);

    stat = xiSetParamInt(xiH, XI_PRM_TRG_SOURCE, XI_TRG_SOFTWARE);
    HandleResult(stat, "xiSetParam (XI_PRM_TRG_SOURCE)");

    stat = xiStartAcquisition(xiH);
    HandleResult(stat, "xiStartAcquisition");

    start_time = clock();

    for (int images = 0; images < EXPECTED_IMAGES; images++)
    {
        // getting image from camera

        stat = xiSetParamInt(xiH, XI_PRM_TRG_SOFTWARE, 1);
        HandleResult(stat, "XI_PRM_TRIG_SOFTWARE");

        stat = xiGetImage(xiH, 5000, &image);
        HandleResult(stat, "xiGetImage");
        printf(".");

    }
    end_time = clock();

    stat = xiStopAcquisition(xiH);
    HandleResult(stat, "xiStopAcquisition");

    t = end_time - start_time;
    seconds = (float)t / CLOCKS_PER_SEC;
    frame_rate = (float)EXPECTED_IMAGES / seconds;
    float cycle_time_soft_trig = 1000 / frame_rate;

    printf("\n cycle_time_soft_trig = %.2f ms frame_rate is %.2f FPS\n\n\n", cycle_time_soft_trig, frame_rate);
    printf("\n overhead = %.2f ms \n", cycle_time_soft_trig - cycle_time_free_run);

finish:
    // Close device
    if (xiH)
        xiCloseDevice(xiH);
    printf("Done\n");

    Sleep(5000);

    return 0;
}

