/*

*/
#include "stdafx.h"
#include "Exported.h"

extern "C"
{

    bool initSystem()
    {
        bool fOK = false;
        return fOK;
    }


#if false
    void printStatusIfNonZero(const char *prefix)
    {
        if (status) printf("%s: %x %s\n", prefix, status, getStatusText(status));
    }


    // 
    // Main Program
    //

    // called once to get a handle to the frame grabber
    // returns true on success

    bool initSystem()
    {
        bool fOK = false;
        // Initialize Sapera API
        if ((status = CorManOpen()) == CORSTATUS_OK) {
            // Get server handles (system and board)
            hSystem = CorManGetLocalServer();
            if ((status = CorManGetServerByName("Xcelera-CL+_PX8_1", &hBoard)) == CORSTATUS_OK) {
                // Get acquisition handle
                if ((status = CorAcqGetHandle(hBoard, 0, &hAcq)) == CORSTATUS_OK) { // 0 = First instance
                    fOK = true;
                }
            }
        }
        printStatusIfNonZero("initSystem");
        return fOK;
    }


    // called once to get handles to the Cam and Vic resources and load cam and vic configuration files
    // returns true on success

    bool loadConfigFiles(const char cam_file[256], const char vic_file[256])
    {
        bool fOK = false;
        if ((status = CorCamNew(hSystem, &hCam)) == CORSTATUS_OK) { // Camera
            printStatusIfNonZero("CorCamNew");
            if ((status = CorVicNew(hSystem, &hVic)) == CORSTATUS_OK) { // Video-Input-Conditionning
                printStatusIfNonZero("CorVicNew");
                // Load CAM/VIC parameters from file into system memory
                // The acquisition hardware is not initialized at this point
                status = CorCamLoad(hCam, cam_file);
                printf("Cam load: %s, %s\n", getStatusText(status), cam_file);
                if (status == CORSTATUS_OK) {
                    status = CorVicLoad(hVic, vic_file);
                    printf("Vic load: %s, %s\n", getStatusText(status), vic_file);
                    if (status == CORSTATUS_OK) {
                        // Download the CAM/VIC parameters to the acquisition module
                        // The acquisition hardware is now initialized
                        if ((status = CorAcqSetPrms(hAcq, hVic, hCam, FALSE)) == CORSTATUS_OK) {
                            printStatusIfNonZero("CorAcqSetPrms");
                            fOK = true;
                        }
                        printf("CorAcqSetPrms: %s\n", getStatusText(status));
                    }
                }
            }
        }

        return fOK;
    }



    // Allocate a single capture buffer and transfer resources to move the grabbed image to system memory

    void createBuffer()
    {
        status = CorAcqGetPrm(hAcq, CORACQ_PRM_SCALE_HORZ, &width);
        status = CorAcqGetPrm(hAcq, CORACQ_PRM_SCALE_VERT, &height);
        status = CorAcqGetPrm(hAcq, CORACQ_PRM_OUTPUT_FORMAT, &format);
        printf("Initializing image buffer...\n");
        printf("\tshape: %dx%d\n", width, height);
        printf("\tformat: %#010x\n", format);
        status = CorBufferNew(hSystem, width, height, format,
            //CORBUFFER_VAL_TYPE_CONTIGUOUS, &hBuffer);
            CORBUFFER_VAL_TYPE_SCATTER_GATHER, &hBuffer);
        printf("Buffer created: %s\n", getStatusText(status));
        status = CorXferNew(hBoard, hAcq, hBuffer, NULL, &hXfer);
        printf("Buffer transfer initialized: %s\n", getStatusText(status));
        status = CorXferConnect(hXfer);
        printf("Buffer transfer connected: %s\n", getStatusText(status));
        // set up output buffer
        imgsize = width * height * sizeof(UINT16);
        pData = (UINT16 *)malloc(imgsize);

    }

    void acquireImages(UINT32 nframe, const char dir_str[256])
    {
        status = CorFileNew(hSystem, dir_str, "w", &hFile);
        status = CorXferStart(hXfer, nframe);
        status = CorXferWait(hXfer, 1000);
        status = CorFileSave(hFile, hBuffer, "-format tiff");
        status = CorFileFree(hFile);
    }

    UINT32 queueFrame()
    {
        status = CorXferStart(hXfer, 1);
        printStatusIfNonZero("CorXferStart");
        return status;
    }

    UINT16 *grabFrame()
    {
        status = CorXferWait(hXfer, 1000);
        printStatusIfNonZero("CorXferWait");
        status = CorBufferRead(hBuffer, 0, pData, imgsize);
        printStatusIfNonZero("CorBufferRead");
        return pData;
    }

    UINT32 getWidth()
    {
        status = CorAcqGetPrm(hAcq, CORACQ_PRM_SCALE_HORZ, &width);
        return width;
    }

    UINT32 getHeight()
    {
        status = CorAcqGetPrm(hAcq, CORACQ_PRM_SCALE_VERT, &height);
        return height;
    }

    UINT32 getFormat()
    {
        status = CorAcqGetPrm(hAcq, CORACQ_PRM_OUTPUT_FORMAT, &format);
        return format;
    }

    UINT32 getBufferDataDepth()
    {
        UINT32 depth;
        status = CorBufferGetPrm(hBuffer, CORBUFFER_PRM_DATADEPTH, &depth);
        return depth;
    }

    UINT32 getBufferPixelDepth()
    {
        UINT32 depth;
        status = CorBufferGetPrm(hBuffer, CORBUFFER_PRM_PIXEL_DEPTH, &depth);
        return depth;
    }

    UINT32 getBufferType()
    {
        UINT32 type;
        status = CorBufferGetPrm(hBuffer, CORBUFFER_PRM_TYPE, &type);
        return type;
    }

    void printStatus(UINT32 status)
    {
        CorManGetStatusText(status, sText, sizeof(sText), NULL, 0);
        printf("Status %d: %s\n", status, sText);
    }

    CHAR *getStatusText(UINT32 status)
    {
        CorManGetStatusText(status, sText, sizeof(sText), NULL, 0);
        return sText;
    }

    UINT32 getParameter(UINT32 prm, UINT32 status)
    {
        status = CorAcqGetPrm(hAcq, prm, &value);
        return value;
    }

    UINT32 setParameter(UINT32 sprm, UINT32 svalue, UINT32 status)
    {
        status = CorAcqSetPrm(hAcq, sprm, svalue);
        return status;
    }

    void freeArray()
    {
        free(pData);
        status = CorBufferFree(hBuffer);
    }
    void disconnectSapera()
    {
        status = CorXferDisconnect(hXfer);
    }

    void freeSapera()
    {
        status = CorXferFree(hXfer);
        status = CorBufferFree(hBuffer);
        status = CorVicFree(hVic);
        status = CorCamFree(hCam);
        status = CorAcqRelease(hAcq);
    }

    void closeSapera()
    {
        // Close Sapera API
        status = CorManClose();
    }

#endif
}