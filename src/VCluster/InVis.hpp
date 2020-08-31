/*
 * InVis.hpp
 *
 *  Created on: May 11, 2020
 *      Author: Aryaman Gupta
 */

#ifndef OPENFPM_PDATA_INVIS_HPP
#define OPENFPM_PDATA_INVIS_HPP

#include <jni.h>
#include <memory/ShmBuffer.hpp>

enum vis_type
{
    particles = 0x0,
    grid = 0x1,
};

static std::string grid_shm_path = "/home/aryaman/temp";
static std::string particle_pos_shm_path = "/";
static std::string particle_prop_shm_path = "/home";
static std::string datatype_path = "/home/aryaman/datatype";

class InVis
{
    int windowSize;
    int computePartners;
    int imageSize;
    JavaVM *jvm;
    jclass clazz;
    jobject obj;
    MPI_Comm visComm;
    MPI_Comm renderComm;
    MPI_Comm steerComm;
    int rCommSize;
    int vCommSize;
    vis_type vtype;
    std::vector<std::vector<ShmBuffer *>> gridBuffers;
    bool vis_is_running;


    void updateMemory(jmethodID methodID, int memKey, bool pos);
    void getMemoryPos();
    void getMemoryProps();
    void receiveImages();
    void updateCamera();
    void interactVis();
    void getGridMemory();
    void doRender();
    static vis_type getVisType();

public:
    InVis(int cPartners, MPI_Comm vComm, MPI_Comm rcomm, MPI_Comm sComm, bool isMaster);
    void manageRenderer();
    void manageMaster();
    };

#endif //OPENFPM_PDATA_InVis_HPP
