/*
 * InVis.hpp
 *
 *  Created on: May 11, 2020
 *      Author: Aryaman Gupta
 */

#ifndef OPENFPM_PDATA_INVIS_HPP
#define OPENFPM_PDATA_INVIS_HPP

#include <jni.h>

class InVis
{
    int windowSize;
    int computePartners;
    int imageSize;
    JavaVM *jvm;
    jclass clazz;
    jobject obj;
    MPI_Comm visComm;
    int commSize;

    void updateMemory(jmethodID methodID, int memKey, bool pos);
    void getMemoryPos();
    void getMemoryProps();
    void receiveImages();
    void updateCamera();
    void doRender();

public:
    InVis(int wSize, int cPartners, MPI_Comm vComm,  bool isHead);
    void manageVisHead();
    void manageVisRenderer();
    };

#endif //OPENFPM_PDATA_InVis_HPP
