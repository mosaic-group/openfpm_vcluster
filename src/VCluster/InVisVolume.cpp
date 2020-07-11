/*
 * InVisVolumeVolume.cpp
 *
 *  Created on: June 18, 2020
 *      Author: Aryaman Gupta
 */

#include <iostream>
#include <jni.h>
#include <thread>
#include <vector>
#include <mpi.h>
#include <dirent.h>
#include <zconf.h>
#include <fstream>

#include "InVisVolume.hpp"
#include "memory/ShmBuffer.hpp"

#define USE_VULKAN true

MPI_Comm mpiComm;
//int imgSize;
double avg_recent = 0.0;
int prevtime = 0;
int count = 0;
int totaltime = 0;
int timeVDIwrite = 0;

InVisVolume::InVisVolume(int wSize, int cPartners, MPI_Comm vComm, bool isHead) {
    windowSize = wSize;
    computePartners = cPartners;
    imageSize = windowSize*windowSize*7;
    visComm = vComm;
    mpiComm = vComm;
    MPI_Comm_size(visComm, &commSize);
    std::cout<<"VisComm size is "<<commSize <<std::endl;
//    imgSize = imageSize;

    for(int i = 0; i< computePartners; i++) {
        std::vector<ShmBuffer *> tmp;
        gridBuffers.push_back(tmp);
    }

    DIR *dir;
    struct dirent *ent;
    std::string classPath = "-Djava.class.path=";
    std::string directory = "/home/aryaman/Repositories/scenery-insitu/target/dependency/";
    if ((dir = opendir ("/home/aryaman/Repositories/scenery-insitu/target/dependency")) != NULL) {
        /* print all the files and directories within directory */
        while ((ent = readdir (dir)) != NULL) {
            classPath.append(directory);
            classPath.append(ent->d_name);
            classPath.append(":");
        }
        closedir (dir);
    } else {
        /* could not open directory */
        std::cerr << "ERROR: Could not open directory "<< directory <<std::endl;
        std::exit(EXIT_FAILURE);
    }

//    std::cout<<"classpath is "<<classPath<<std::endl;

//    ================== prepare loading of Java VM ============================
    JavaVMInitArgs vm_args;                        // Initialization arguments
    auto *options = new JavaVMOption[7];   // JVM invocation options
    options[0].optionString = (char *)classPath.c_str();

    if(USE_VULKAN) {
        options[1].optionString = (char *)
                "-Dscenery.Renderer=VulkanRenderer";
    }
    else {
        options[1].optionString = (char *)
                "-Dscenery.Renderer=OpenGLRenderer";
    }

    if(isHead) {
        options[2].optionString = (char *)
                "-Dorg.slf4j.simpleLogger.defaultLogLevel=info";
    }
    else {
        options[2].optionString = (char *)
                "-Dorg.slf4j.simpleLogger.defaultLogLevel=info";
    }

    options[3].optionString = (char *)
            "-Dscenery.Headless=true";
    options[4].optionString = (char *)
            "-Dscenery.Renderer.MaxVolumeCacheSize=16";
    options[5].optionString = (char *)
            "-Dscenery.VulkanRenderer.EnableValidations=false";
    options[6].optionString = (char *)
            "-Dscenery.VulkanRenderer.StrictValidation=false";


    vm_args.version = JNI_VERSION_1_6;             // minimum Java version
    vm_args.nOptions = 7;                          // number of options
    vm_args.options = options;
    vm_args.ignoreUnrecognized = false;     // invalid options make the JVM init fail
    //=============== load and initialize Java VM and JNI interface =============
    JNIEnv *env;
    jint rc = JNI_CreateJavaVM(&jvm, (void **) &env, &vm_args);  // YES !!
    std::cout<<"Hello world"<<std::endl;
    delete options;    // we then no longer need the initialisation options.
//    if (rc != JNI_OK) {
//        // TODO: error processing
//        std::cin.get();
//        std::exit(EXIT_FAILURE);
//    }

    //=============== Display JVM version =======================================
    std::cout << "JVM load succeeded: Version " << std::endl;
    jint ver = env->GetVersion();
    std::cout << ((ver >> 16) & 0x0f) << "." << (ver & 0x0f) << std::endl;

    std::string className;
    if(isHead) className = "graphics/scenery/insitu/Head";
    else className = "graphics/scenery/insitu/InVisVolumeRenderer";

    jclass localClass;
    localClass = env->FindClass(className.c_str());  // try to find the class
    if(localClass == nullptr) {
        if( env->ExceptionOccurred() ) {
            env->ExceptionDescribe();
        } else {
            std::cerr << "ERROR: class "<< className << " not found !" <<std::endl;
            std::exit(EXIT_FAILURE);
        }
    }

    // if class found, continue
    std::cout << "Class found " << className << std::endl;
    jmethodID ctor = env->GetMethodID(localClass, "<init>", "()V");  // FIND AN OBJECT CONSTRUCTOR
    if (ctor == nullptr) {
        if( env->ExceptionOccurred() ) {
            env->ExceptionDescribe();
        } else {
            std::cerr << "ERROR: constructor not found !" <<std::endl;
            std::exit(EXIT_FAILURE);
        }
    }

    //if constructor found, continue
    jobject localObj;
    localObj = env->NewObject(localClass, ctor);
    if (!localObj) {
        if (env->ExceptionOccurred()) {
            env->ExceptionDescribe();
        } else {
            std::cout << "ERROR: class object is null !";
            std::exit(EXIT_FAILURE);
        }
    }

//    std::cout << "Object of class has been constructed" << std::endl;

    clazz = reinterpret_cast<jclass>(env->NewGlobalRef(localClass));
    obj = reinterpret_cast<jobject>(env->NewGlobalRef(localObj));

    env->DeleteLocalRef(localClass);
    env->DeleteLocalRef(localObj);

//    std::cout << "Global references have been created and local ones deleted" << std::endl;
}

void InVisVolume::getMemory() {
    JNIEnv *env;
    jvm->AttachCurrentThread(reinterpret_cast<void **>(&env), NULL);

    jmethodID updateDataMethod = env->GetMethodID(clazz, "updateData", "(II[Ljava/nio/ByteBuffer;[I[I[I)V");
    if(updateDataMethod == nullptr) {
        if (env->ExceptionOccurred()) {
            env->ExceptionDescribe();
        } else {
            std::cout << "ERROR: function updateData not found!";
//            std::exit(EXIT_FAILURE);
        }
    }

//    std::cout<<"GOing to sleep"<<std::endl;
//    sleep(5);
    for(int i = 0; i < computePartners; i++) {
        // Read the metadata of the grids managed by computePartner i
        std::cout<<"Attempting to read metadata managed by computePartner " << i << std::endl;
        ShmBuffer *GBoxBuf;
        std::string pName = "/home/aryaman/temp" + std::to_string(i);

        GBoxBuf = new ShmBuffer(pName, 0, true);
        ptr_with_size pws{};

        std::cout<<"Calling update key function"<<std::endl;
        GBoxBuf->update_key(true);
        pws = GBoxBuf->attach();

        size_t metaSize = pws.size - 512 - 32;
        int numMetaElements  = metaSize / sizeof(long);
        std::cout << "Got metadata for compute partner " << i << ". The size and no of elements are:" << metaSize << " and " << numMetaElements << std::endl;
        long *metaData = (long*)pws.ptr;

        for(int j = 0; j < numMetaElements; j++) {
            std::cout<<j+1<<"th element is "<<metaData[j]<<"\t";
        }
        std::cout<<std::endl;

        int elementsInGBox = 15; // origin -> 3 + grid dimensions -> 6 + domain dimensions -> 6
        int numGrids = numMetaElements / elementsInGBox;

        int origins[3 * numGrids];
        jintArray jOrigins = env->NewIntArray(3 * numGrids);
        int gridDims[6 * numGrids];
        jintArray jGridDims = env->NewIntArray(6 * numGrids);
        int domainDims[6 * numGrids];
        jintArray jDomainDims = env->NewIntArray(6 * numGrids);

        jobjectArray bytebuffers; // Each element will be a direct byte buffer wrapping a grid that is controlled by this computePartner
        jsize size = numGrids;
        bytebuffers = (env)->NewObjectArray(size, (env)->FindClass("java/nio/ByteBuffer"), 0);

        int it = 0;
        for(int j = 0; j < numGrids; j++) {
            gridDims[j * 6] = (int)metaData[it];
            it ++;
            gridDims[j * 6 + 1] = (int)metaData[it];
            it++;
            gridDims[j * 6 + 2] = (int)metaData[it];
            it++;
            gridDims[j * 6 + 3] = (int)metaData[it];
            it ++;
            gridDims[j * 6 + 4] = (int)metaData[it];
            it++;
            gridDims[j * 6 + 5] = (int)metaData[it];
            it++;

            domainDims[j * 6] = (int)metaData[it];
            it ++;
            domainDims[j * 6 + 1] = (int)metaData[it];
            it++;
            domainDims[j * 6 + 2] = (int)metaData[it];
            it++;
            domainDims[j * 6 + 3] = (int)metaData[it];
            it ++;
            domainDims[j * 6 + 4] = (int)metaData[it];
            it++;
            domainDims[j * 6 + 5] = (int)metaData[it];
            it++;

            origins[j * 3] = (int)metaData[it];
            it ++;
            origins[j * 3 + 1] = (int)metaData[it];
            it++;
            origins[j * 3 + 2] = (int)metaData[it];
            it++;

            gridBuffers[i].push_back(new ShmBuffer(pName, j+1, true));
            gridBuffers[i][j]->update_key(true);

            ptr_with_size tmp = gridBuffers[i][j]->attach();
            int width = abs(gridDims[j*6] - gridDims[j*6 + 3]) + 1;
            int height = abs(gridDims[j*6 + 1] - gridDims[j*6 + 4]) + 1;
            int depth = abs(gridDims[j*6 + 2] - gridDims[j*6 + 5]) + 1;
            int gridSize = width * height * depth;
            std::cout<< "Grid " << j << " of computePartner " << i << " is of size " << gridSize << std::endl;

            jobject bb;
            bb = env->NewDirectByteBuffer((void *)tmp.ptr, 2*gridSize);
            env->SetObjectArrayElement(bytebuffers, j, bb);
            int temp = (env)-> GetDirectBufferCapacity(bb);
            std::cout<<"Bytebuffer capacity was found to be " <<temp <<std::endl;
//            env->DeleteLocalRef(bb); TODO: uncomment
        }

        env->SetIntArrayRegion(jOrigins, 0, 3 * numGrids, origins);
        env->SetIntArrayRegion(jGridDims, 0, 6 * numGrids, gridDims);
        env->SetIntArrayRegion(jDomainDims, 0, 6 * numGrids, domainDims);
        // the meta arguments for the java function updateData are now ready
        std::cout<<"the meta arguments for the java function updateData are now ready"<<std::endl;

        env->CallVoidMethod(obj, updateDataMethod, i, numGrids, bytebuffers, jOrigins, jGridDims, jDomainDims);

//        env->DeleteLocalRef(bytebuffers);
//        env->DeleteLocalRef(jOrigins);
//        env->DeleteLocalRef(jGridDims);
//        env->DeleteLocalRef(jDomainDims);
    }
    jvm->DetachCurrentThread();
}

void InVisVolume::doRender() {
    JNIEnv *env;
    jvm->GetEnv((void **)&env, JNI_VERSION_1_6);

    jmethodID mainMethod = env->GetMethodID(clazz, "main", "()V");
    if(mainMethod == nullptr) {
        if (env->ExceptionOccurred()) {
            env->ExceptionDescribe();
        } else {
            std::cout << "ERROR: function main not found!";
//            std::exit(EXIT_FAILURE);
        }
    }
    env->CallVoidMethod(obj, mainMethod);
    if(env->ExceptionOccurred()) {
        env->ExceptionDescribe();
        env->ExceptionClear();
    }
}


void distributeVDIs(JNIEnv *e, jobject clazzObject, jobject subVDICol, jobject subVDIDepth, jint sizePerProcess, jint commSize) {

    void *ptrCol = e->GetDirectBufferAddress(subVDICol);
    void *ptrDepth = e->GetDirectBufferAddress(subVDIDepth);

    void *recvBufCol = malloc(sizePerProcess * commSize);
    void *recvBufDepth = malloc(sizePerProcess * commSize * 2);

    MPI_Alltoall(ptrCol, sizePerProcess, MPI_BYTE, recvBufCol, sizePerProcess, MPI_BYTE, mpiComm);
    MPI_Alltoall(ptrDepth, sizePerProcess*2, MPI_BYTE, recvBufDepth, sizePerProcess*2, MPI_BYTE, mpiComm);

    jclass clazz = e->GetObjectClass(clazzObject);
    jmethodID compositeMethod = e->GetMethodID(clazz, "compositeVDIs", "(Ljava/nio/ByteBuffer;Ljava/nio/ByteBuffer;I)V");

    jobject bbCol = e->NewDirectByteBuffer(recvBufCol, sizePerProcess * commSize);
    jobject bbDepth = e->NewDirectByteBuffer(recvBufDepth, sizePerProcess * commSize * 2);

    if(e->ExceptionOccurred()) {
        e->ExceptionDescribe();
        e->ExceptionClear();
    }

    std::cout<<"Finished distributing the VDIs. Calling the Composite method now!"<<std::endl;

    e->CallVoidMethod(clazzObject, compositeMethod, bbCol, bbDepth, sizePerProcess);
    if(e->ExceptionOccurred()) {
        e->ExceptionDescribe();
        e->ExceptionClear();
    }
}

void gatherCompositedVDIs(JNIEnv *e, jobject clazzObject, jobject compositedVDIColor, jobject compositedVDIDepth, jint root, jint compositedVDILen, jint myRank, jint commSize, jboolean saveFiles) {

    std::cout<<"In Gather function " <<std::endl;
    void *ptrCol = e->GetDirectBufferAddress(compositedVDIColor);
    void *ptrDepth = e->GetDirectBufferAddress(compositedVDIDepth);
//    std::cout << "Data received as parameter is: " << (char *)ptrCol << std::endl;
    void *recvBufCol;
    void *recvBufDepth;
    if (myRank == 0) {
        recvBufCol = malloc(compositedVDILen * commSize);
        recvBufDepth = malloc(compositedVDILen * commSize * 2);
    }
    else {
        recvBufCol = NULL;
        recvBufDepth = NULL;
    }

    MPI_Gather(ptrCol, compositedVDILen, MPI_BYTE, recvBufCol, compositedVDILen, MPI_BYTE, root, mpiComm);
    MPI_Gather(ptrDepth, compositedVDILen*2, MPI_BYTE, recvBufDepth, compositedVDILen*2, MPI_BYTE, root, mpiComm);
    //The data is here now!

    if(myRank == 0) {
//        //send or store the VDI
        std::time_t t = std::time(0);

        int timetaken = 0;
        if(prevtime == 0) {
            //skip this step
        }
        else {
            timetaken = t - prevtime;
            totaltime += timetaken;
            avg_recent += timetaken;
            count++;
        }
        if(count != 0 && count % 20 == 0) {
            double average_time = (double)totaltime/count;
            avg_recent = (double)avg_recent/20.0;
            std::cout<<"Average time for the last 20 VDIs was: "<<avg_recent<<std::endl;
            avg_recent = 0.0;
            std::cout<<"Overall average so far is"<<average_time<<" and total VDIs generated so far are "<< count <<std::endl;

        }
        if(count != 0 && count % 20 == 0) {
            if(timeVDIwrite != 0) {
                int time_elapsed = t - timeVDIwrite;
                std::cout<<"Writing VDI now. Time since the last write was performed is: " << time_elapsed <<std::endl;
                timeVDIwrite = t;
            }
            std::cout<<"Writing the final gathered VDI now"<<std::endl;

            std::string filename = "size:" + std::to_string(commSize) + "Final_VDICol" + std::to_string(count) + ".raw";
            std::ofstream b_stream(filename.c_str(),
                                   std::fstream::out | std::fstream::binary);
            std::string filenameDepth = "Final_VDIDepth" + std::to_string(count) + ".raw";
            std::ofstream b_streamDepth(filenameDepth.c_str(),
                                   std::fstream::out | std::fstream::binary);

            if (b_stream && b_streamDepth)
            {
                b_stream.write(static_cast<const char *>(recvBufCol), compositedVDILen * commSize);
                b_streamDepth.write(static_cast<const char *>(recvBufDepth), compositedVDILen * commSize * 2);

                if (b_stream.good() && b_streamDepth.good()) {
                    std::cout<<"Writing was successful"<<std::endl;
                }
            }

        }
        std::time_t t_out = std::time(0);
        prevtime = t_out;
//        std::cout << "cpp on rank " << myRank << " color data received is " << (char *)recvBufCol << std::endl;
//        std::cout << "cpp on rank " << myRank << " depth data received is " << (char *)recvBufDepth << std::endl;
    }

//    jclass clazz = e->GetObjectClass(clazzObject);
//    jmethodID updateMethod = e->GetMethodID(clazz, "updateVolumes", "()V");
//
//    e->CallVoidMethod(clazzObject, updateMethod);
//    if(e->ExceptionOccurred()) {
//        e->ExceptionDescribe();
//        e->ExceptionClear();
//    }
}

void InVisVolume::manageVolumeRenderer() {
    JNIEnv *env;
    jvm->GetEnv((void **)&env, JNI_VERSION_1_6);
    jfieldID winSizeField = env->GetFieldID(clazz, "windowSize", "I");
    env->SetIntField(obj, winSizeField, windowSize);

    jfieldID compPartnersField = env->GetFieldID(clazz, "computePartners", "I");
    env->SetIntField(obj, compPartnersField, computePartners);

    jfieldID commSizeField = env->GetFieldID(clazz, "commSize", "I");
    env->SetIntField(obj, commSizeField, commSize);

    int visRank;
    MPI_Comm_rank(visComm, &visRank);

    jfieldID rankField = env->GetFieldID(clazz, "rank", "I");
    env->SetIntField(obj, rankField, visRank);

    jmethodID initArraysMethod = env->GetMethodID(clazz, "initializeArrays", "()V");

    if(initArraysMethod == nullptr) {
        if (env->ExceptionOccurred()) {
            env->ExceptionDescribe();
        } else {
            std::cout << "ERROR: function initializeArrays not found!";
//            std::exit(EXIT_FAILURE);
        }
    }
    env->CallVoidMethod(obj, initArraysMethod);

    JNINativeMethod methods[] { { (char *)"distributeVDIs", (char *)"(Ljava/nio/ByteBuffer;Ljava/nio/ByteBuffer;II)V", (void *)&distributeVDIs },
            { (char *)"gatherCompositedVDIs", (char *)"(Ljava/nio/ByteBuffer;Ljava/nio/ByteBuffer;IIIIZ)V", (void *)&gatherCompositedVDIs }
    };  // mapping table

    if(env->RegisterNatives(clazz, methods, 2) < 0) {                        // register it
        if( env->ExceptionOccurred() ) {
            env->ExceptionDescribe();
        } else {
            std::cerr << "ERROR: Could not register natives for InVisVolume!" <<std::endl;
            //std::exit(EXIT_FAILURE);
        }
    } else {
        std::cout<<"Natives registered for InVisVolume. The return value is: "<< env->RegisterNatives(clazz, methods, 1) <<std::endl;
    }

    std::cout<<"starting thread: updatePosData"<<std::endl;

    std::thread updatePosData(&InVisVolume::getMemory, this);

    std::cout<<"calling function: doRender"<<std::endl;

    doRender();

    std::cout<<"Finished with doRender!"<<std::endl;

    updatePosData.join();

    jvm->DestroyJavaVM();
}
