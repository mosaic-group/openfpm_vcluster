/*
 * InVisVolume.hpp
 *
 *  Created on: June 18, 2020
 *      Author: Aryaman Gupta
 */

#ifndef SRC_INVISVOLUME_HPP
#define SRC_INVISVOLUME_HPP

#include <jni.h>
#include <mpi.h>
#include <memory/ShmBuffer.hpp>

class InVisVolume
{
    int windowSize;
    int computePartners;
    int imageSize;
    JavaVM *jvm;
    jclass clazz;
    jobject obj;
    MPI_Comm visComm;
    int commSize;
    std::vector<std::vector<ShmBuffer *>> gridBuffers;

    void updateMemory(jmethodID methodID, int memKey, bool pos);
    void getMemory();
    void receiveVDI();
    void updateCamera();
    void doRender();

public:
    InVisVolume(int wSize, int cPartners, MPI_Comm vComm,  bool isHead);
    void manageVisHead();
    void manageVolumeRenderer();
};

#endif //SRC_INVISVOLUME_HPP
