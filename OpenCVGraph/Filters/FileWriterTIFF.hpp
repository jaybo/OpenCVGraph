#pragma once

#ifndef INCLUDE_OCVG_FILEWRITERTIFF
#define INCLUDE_OCVG_FILEWRITERTIFF

#include "..\stdafx.h"
#include "..\..\TIFF\include\tiffio.h"

using namespace cv;
using namespace std;

namespace openCVGraph
{
    // Simplest possible filter
    class FileWriterTIFF : public Filter
    {
    public:

        FileWriterTIFF::FileWriterTIFF(std::string name, GraphData& graphData,
            bool showView = true, int width = 512, int height = 512)
            : Filter(name, graphData, width, height)
        {
        }

        bool FileWriterTIFF::processKeyboard(GraphData& data, int key) override
        {
            bool fOK = true;
            char c[2];
            c[0] = char(key);
            c[1] = 0;
            if (string(c) == m_WriteOnKeyHit) {
                m_WriteNextImage = true;    // Trigger write
            }
            return fOK;
        }

        //Allocate resources if needed, and specify the image formats required
        bool FileWriterTIFF::init(GraphData& graphData) override
        {
            // call the base to read/write configs
            Filter::init(graphData);

            if (m_Enabled) {

                graphData.m_NeedCV_16UC1 = true;

                createDir(m_Directory);

                if (m_WriteOnKeyHit != "") {
                    m_WriteNextImage = false;
                }
            }
            return true;
        }

        // Do all of the work here.
        ProcessResult FileWriterTIFF::process(GraphData& graphData) override
        {
            if (m_WriteNextImage) {
                string fullName = m_Directory + '/' + m_Name + std::to_string(graphData.m_FrameNumber) + m_Ext;
                
                int w, h;
                uint16 samplesperpixel;
                uint16 bitspersample;
                uint32 rowsperstrip = (uint32)-1;

                w = graphData.m_imOut16UC1.size().width;
                h = graphData.m_imOut16UC1.size().height;
                samplesperpixel = 1;
                bitspersample = 16;
                unsigned char *outbuf;

                TIFF* out = TIFFOpen(fullName.c_str(), "w");

                TIFFSetField(out, TIFFTAG_IMAGEWIDTH, w);
                TIFFSetField(out, TIFFTAG_IMAGELENGTH, h);
                TIFFSetField(out, TIFFTAG_BITSPERSAMPLE, bitspersample);
                TIFFSetField(out, TIFFTAG_SAMPLESPERPIXEL, samplesperpixel);
                TIFFSetField(out, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
                TIFFSetField(out, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
                TIFFSetField(out, TIFFTAG_IMAGEDESCRIPTION, "test");
                TIFFSetField(out, TIFFTAG_SOFTWARE, "openCVGraph");
                outbuf = (unsigned char *)_TIFFmalloc(TIFFScanlineSize(out));
                TIFFSetField(out, TIFFTAG_ROWSPERSTRIP,
                    TIFFDefaultStripSize(out, rowsperstrip));

                // inbuf = (unsigned char *)_TIFFmalloc(TIFFScanlineSize(in));
                for (int row = 0; row < h; row++) {
                    if (TIFFWriteScanline(out, outbuf, row, 0) < 0)
                        break;
                }

                _TIFFfree(outbuf);
                TIFFClose(out);

                if (m_WriteOnKeyHit != "") {
                    m_WriteNextImage = false;
                }

            }
            return ProcessResult::OK;  // if you return false, the graph stops
        }

        void  FileWriterTIFF::saveConfig(FileStorage& fs, GraphData& data) override
        {
            Filter::saveConfig(fs, data);
            fs << "directory" << m_Directory.c_str();
            fs << "name" << m_Name.c_str();
            fs << "ext" << m_Ext.c_str();
            cvWriteComment((CvFileStorage *)*fs, "Set writeOnKeyHit to a single char to trigger write if that key is hit. Leave empty to write every frame.", 0);
            fs << "writeOnKeyHit" << m_WriteOnKeyHit.c_str();
        }

        void  FileWriterTIFF::loadConfig(FileNode& fs, GraphData& data) override
        {
            Filter::loadConfig(fs, data);
            fs["directory"] >> m_Directory;
            fs["name"] >> m_Name;
            fs["ext"] >> m_Ext;
            fs["writeOnKeyHit"] >> m_WriteOnKeyHit;
        }

    private:
        string m_Directory = "C:/junk";
        string m_Name = "test";
        string m_Ext = ".tif";
        string m_WriteOnKeyHit = "";
        bool m_WriteNextImage = true;
    };
}
#endif