#pragma once

#include "stdafx.h"

using namespace std;
using namespace openCVGraph;

//Pixels                         : 14745600
//Mat dimensions: 3840x3840 CV_16UC1

// Jay's Lenovo Desktop at work
//
//m1 = m1 * 16                   : 0.0266
//mm[x][y] <<= 4                 : 0.0347
//gm1.upload(m1)                 : 0.0232
//gm1.download(m1)               : 0.0220
//cuda::add(gm1, gm2, gm1)       : 0.0005
//cuda::lshift(gm1, 4, gm1)      : 0.0006
//cuda::multiply (gm1, 16, gm1)  : 0.0004
//cuda::divide(gm1, 16, gm2)     : 0.0006

// Jay's home system i7 with GeForce GTX 970
//
//m1 = m1 * 16                   : 0.0326
//mm[x][y] <<= 4                 : 0.0473
//gm1.upload(m1)                 : 0.0090
//gm1.download(m1)               : 0.0089
//cuda::add(gm1, gm2, gm1)       : 0.0007
//cuda::lshift(gm1, 4, gm1)      : 0.0005
//cuda::multiply (gm1, 16, gm1)  : 0.0005
//cuda::divide(gm1, 16, gm2)     : 0.0005

// Jay's Lenovo Carbon X1 laptop
//
//m1 = m1 * 16                   : 0.0254
//mm[x][y] <<= 4                 : 0.0347


// Time some common Mat and GpuMat operations
//
void timeMatOps(int v = 3840)
{
    int nLoopCount;
    const int d = v;

    // uint16_t *a = new uint16_t[d*d];
    uint16_t * mm = new uint16_t[d *d];

    Mat m1(d, d, CV_16UC1);
    Mat m2(d, d, CV_16UC1);
    cv::randu(m1, Scalar::all(0), Scalar::all(65535));
    cv::randu(m2, Scalar::all(0), Scalar::all(65535));
#ifdef WITH_CUDA
    GpuMat gm1(m1);
    GpuMat gm2(m1);
#endif
    auto timer = Timer();
    nLoopCount = 100;

    cout << std::endl << "Mat dimensions: " << d << "x" << d << " CV_16UC1" << std::endl << std::endl;
    cout << "Pixels                         : " << fixed << (d*d) << std::endl;
    timer.reset();
    for (int i = 0; i < nLoopCount; i++) {
        // 27 mS
        m1 = m1 * 16;
    }
    cout << "m1 = m1 * 16                   : " << fixed << setprecision(4) << (timer.elapsed() / nLoopCount) << std::endl;

    timer.reset();
    for (int i = 0; i < nLoopCount; i++) {
        // 34mS
        for (int x = 0; x < d*d; x++) {
            mm[x] <<= 4;
        }
    }
    cout << "mm[x][y] <<= 4                 : " << fixed << setprecision(4) << (timer.elapsed() / nLoopCount) << std::endl;

#ifdef WITH_CUDA
    timer.reset();
    for (int i = 0; i < nLoopCount; i++) {
        // 28mS
        gm1.upload(m1);
    }
    cout << "gm1.upload(m1)                 : " << fixed << setprecision(4) << (timer.elapsed() / nLoopCount) << std::endl;

    timer.reset();
    for (int i = 0; i < nLoopCount; i++) {
        // 22mS
        gm1.download(m1);
    }
    cout << "gm1.download(m1)               : " << fixed << setprecision(4) << (timer.elapsed() / nLoopCount) << std::endl;

    nLoopCount = 1000;

    timer.reset();
    for (int i = 0; i < nLoopCount; i++) {
        cuda::add(gm1, gm2, gm1);
    }
    cout << "cuda::add(gm1, gm2, gm1)       : " << fixed << setprecision(4) << (timer.elapsed() / nLoopCount) << std::endl;

    timer.reset();
    for (int i = 0; i < nLoopCount; i++) {
        // 0.9mS
        cuda::lshift(gm1, 4, gm1);
    }
    cout << "cuda::lshift(gm1, 4, gm1)      : " << fixed << setprecision(4) << (timer.elapsed() / nLoopCount) << std::endl;

    timer.reset();
    for (int i = 0; i < nLoopCount; i++) {
        // 0.3mS
        cuda::multiply(gm1, 16, gm1);
    }
    cout << "cuda::multiply (gm1, 16, gm1)  : " << fixed << setprecision(4) << (timer.elapsed() / nLoopCount) << std::endl;

    timer.reset();
    for (int i = 0; i < nLoopCount; i++) {
        // 0.4mS
        cuda::divide(gm1, 16, gm2);
    }
    cout << "cuda::divide(gm1, 16, gm2)     : " << fixed << setprecision(4) << (timer.elapsed() / nLoopCount) << std::endl;
#endif
    delete[] mm;
}
