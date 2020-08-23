/*
 * InVis.cpp
 *
 *  Created on: May 12, 2020
 *      Author: Aryaman Gupta
 */

#include <iostream>
#include <jni.h>
#include <thread>
#include <vector>
#include <mpi.h>
#include <dirent.h>
#include <fstream>


#include "InVis.hpp"
#include "memory/ShmBuffer.hpp"

#define USE_VULKAN true

MPI_Comm renComm;
int imgSize;
double avg_recent = 0.0;
int prevtime = 0;
int count = 0;
int totaltime = 0;
int timeVDIwrite = 0;

InVis::InVis(int cPartners, MPI_Comm vComm, MPI_Comm rComm) {
    windowSize = 700;
    computePartners = cPartners;
    imageSize = windowSize*windowSize*7; //RGBA + 4 bytes for depth
    visComm = vComm;
    renderComm = rComm;
    renComm = rComm;
    MPI_Comm_size(renderComm, &rCommSize);
    std::cout << "RenderComm size is " << rCommSize << std::endl;
    MPI_Comm_size(visComm, &vCommSize);
    std::cout << "VisComm size is " << vCommSize << std::endl;
    imgSize = imageSize;

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

    std::cout<<"classpath is "<<classPath<<std::endl;

    // determine whether particle or grid data is to be rendered
    vtype = getVisType();

    std::cout<<"Got the data type "<<std::endl;

    std::string className;
    if(vtype == vis_type::grid) className = "graphics/scenery/insitu/InVisSimpleVolumeRenderer";
    else className = "graphics/scenery/insitu/InVisRenderer";

    //================== prepare loading of Java VM ============================
    JavaVMInitArgs vm_args;                        // Initialization arguments
    auto *options = new JavaVMOption[4];   // JVM invocation options
    options[0].optionString = (char *)classPath.c_str();

    if(USE_VULKAN) {
        options[1].optionString = (char *)
                "-Dscenery.Renderer=VulkanRenderer";
    }
    else {
        options[1].optionString = (char *)
                "-Dscenery.Renderer=OpenGLRenderer";
    }

    options[2].optionString = (char *)
            "-Dorg.slf4j.simpleLogger.defaultLogLevel=info";
//        options[3].optionString = (char *)
//                "-Dscenery.ENABLE_VULKAN_RENDERDOC_CAPTURE=1";

    options[3].optionString = (char *)
            "-Dscenery.Headless=false";

    vm_args.version = JNI_VERSION_1_6;
    vm_args.nOptions = 4;
    vm_args.options = options;
    vm_args.ignoreUnrecognized = false;

    JNIEnv *env;
    jint rc = JNI_CreateJavaVM(&jvm, (void **) &env, &vm_args);  // YES !!
    std::cout<<"Hello world"<<std::endl;
    delete options;
    if (rc != JNI_OK) {
        // TODO: error processing...
        std::cin.get();
        std::exit(EXIT_FAILURE);
    }

    std::cout << "JVM load succeeded: Version " << std::endl;
    jint ver = env->GetVersion();
    std::cout << ((ver >> 16) & 0x0f) << "." << (ver & 0x0f) << std::endl;


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
    jmethodID ctor = env->GetMethodID(localClass, "<init>", "()V");  // find constructor
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

    std::cout << "Object of class has been constructed" << std::endl;

    clazz = reinterpret_cast<jclass>(env->NewGlobalRef(localClass));
    obj = reinterpret_cast<jobject>(env->NewGlobalRef(localObj));

    env->DeleteLocalRef(localClass);
    env->DeleteLocalRef(localObj);

    std::cout << "Global references have been created and local ones deleted" << std::endl;
}

vis_type InVis::getVisType() {
    ShmBuffer * buf = new ShmBuffer(datatype_path, 0, false);
    buf->update_key(true);
    ptr_with_size pws = buf->attach();
    int * ptr = (int *)pws.ptr;
    if(*ptr == 1) {
        return vis_type::grid;
    } else if (*ptr == 2) {
        return  vis_type::particles;
    }
    else {
        std::cerr<<"Found unidentified data when searching for data type flag"<<std::endl;
        std::exit(EXIT_FAILURE);
    }
}

void InVis::updateMemory(jmethodID methodID, int memKey, bool pos) {
    JNIEnv *env;
    jvm->AttachCurrentThread(reinterpret_cast<void **>(&env), NULL);
    std::string pName;
    if(pos) pName = "/";
    else pName = "/home";

    ShmBuffer *buf;
    buf = new ShmBuffer(pName, memKey, true);
    double *str = NULL;
    ptr_with_size pws{};
    jobject byteBuffer;

    while(true) {
        buf->update_key(true);
        pws = buf->attach();
        str = (double *)pws.ptr;
        byteBuffer = (env)->NewDirectByteBuffer((void*) str, pws.size);
        int temp = (env)-> GetDirectBufferCapacity(byteBuffer);
        if (temp == -1) {
            std::cout<<"Bytebuffer capacity was found to be -1. Size of mem returned was " << pws.size <<". This is for memKey " << memKey << " and pName: " << pName << std::endl;
        }
        std::cout<<"Java method updatepos/updateprops has been called by thread with memkey " << memKey <<"! with buffer size "<<temp<<std::endl;

        env->CallVoidMethod(obj, methodID, byteBuffer, memKey);
        std::cout<<"Back after updating pos/props"<<std::endl;
        buf->detach(false); //detach from the previous key
        env->DeleteLocalRef(byteBuffer);
    }
    jvm->DetachCurrentThread();
}

void InVis::getMemoryPos() {
    JNIEnv *env;
    jvm->AttachCurrentThread(reinterpret_cast<void **>(&env), NULL);

    jmethodID updatePosMethod = env->GetMethodID(clazz, "updatePos", "(Ljava/nio/ByteBuffer;I)V");
    if(updatePosMethod == nullptr) {
        if (env->ExceptionOccurred()) {
            env->ExceptionDescribe();
        } else {
            std::cout << "ERROR: function updatePos not found!";
//            std::exit(EXIT_FAILURE);
        }
    }

    std::vector<std::thread> threads;

    for(int i = 0; i < computePartners; i++){
        threads.emplace_back(&InVis::updateMemory, this, updatePosMethod, i, true);
    }

    for(int i = 0; i < computePartners; i++) {
        threads[i].join();
    }
    jvm->DetachCurrentThread();
}

void InVis::getMemoryProps() {
    JNIEnv *env;
    jvm->AttachCurrentThread(reinterpret_cast<void **>(&env), NULL);

    jmethodID updatePropsMethod = env->GetMethodID(clazz, "updateProps", "(Ljava/nio/ByteBuffer;I)V");
    if(updatePropsMethod == nullptr) {
        if (env->ExceptionOccurred()) {
            env->ExceptionDescribe();
        } else {
            std::cout << "ERROR: function updateProps not found!";
//            std::exit(EXIT_FAILURE);
        }
    }

    std::vector<std::thread> threads;

    for(int i = 0; i < computePartners; i++){
        threads.emplace_back(&InVis::updateMemory, this, updatePropsMethod, i, false);
    }

    for(int i = 0; i < computePartners; i++) {
        threads[i].join();
    }
    jvm->DetachCurrentThread();
}

void sendImage(JNIEnv *e, jobject clazz, jobject image) {
    std::cout<< "In send image function" << std::endl;
    jbyte *buf;
    MPI_Request req;
    MPI_Status stat;
    buf = (jbyte *) e->GetDirectBufferAddress(image);
    MPI_Isend((void *)buf, imgSize, MPI_BYTE, 0, 0, renComm, &req);
    MPI_Wait(&req, &stat); //TODO: put wait before Isend or remove wait
}

void InVis::receiveImages() {
    JNIEnv *env;
    jvm->AttachCurrentThread(reinterpret_cast<void **>(&env), NULL);

    char buf[rCommSize - 1][imageSize];
    MPI_Request requests[rCommSize - 1];
    MPI_Status statuses[rCommSize - 1];
    jobjectArray bytebuffers;
    jsize size = rCommSize - 1;
    bytebuffers = (env)->NewObjectArray(size, (env)->FindClass("java/nio/ByteBuffer"), 0);
    std::cout << "Looking for function composite"<<std::endl;

    jmethodID compositeMethod = (env)->GetMethodID(clazz, "composite", "([Ljava/nio/ByteBuffer;I)V");
    if(compositeMethod == nullptr) {
        if ((env)->ExceptionOccurred()) {
            (env)->ExceptionDescribe();
        } else {
            std::cout << "ERROR: function composite not found!"<<std::endl;
//            std::exit(EXIT_FAILURE);
        }
    }

    std::cout << "Function composite has been found!"<<std::endl;

    while (true) {
        for(int i = 1; i < rCommSize; i++) {
            MPI_Irecv(buf[i-1], imageSize, MPI_BYTE, i, 0, visComm, &requests[i-1]);
        }
        MPI_Waitall(rCommSize - 1, requests, statuses);
        jobject bb;
        for(int i = 1; i < rCommSize; i++) {
            bb = (env)->NewDirectByteBuffer((void*) buf[i-1], imageSize);
//            int temp = (env)-> GetDirectBufferCapacity(bb);
//            std::cout<<"Bytebuffer containing color and depth is of size "<<temp<<std::endl;
            (env)->SetObjectArrayElement(bytebuffers, i-1, bb);
        }
        std::cout << "Function composite will be called"<<std::endl;
        (env)-> CallVoidMethod(obj, compositeMethod, bytebuffers, rCommSize);
    }
    jvm->DetachCurrentThread();
}

void InVis::updateCamera() {
    JNIEnv *env;
    jvm->AttachCurrentThread(reinterpret_cast<void **>(&env), NULL);
    jmethodID adjustCamMethod = env->GetMethodID(clazz, "adjustCamera", "()V");
    if(adjustCamMethod == nullptr) {
        if (env->ExceptionOccurred()) {
            env->ExceptionDescribe();
        } else {
            std::cout << "ERROR: function adjustCamera not found!";
//            std::exit(EXIT_FAILURE);
        }
    }
    env->CallVoidMethod(obj, adjustCamMethod);
    jvm->DetachCurrentThread();
}

void InVis::getGridMemory() {
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
        std::string pName = grid_shm_path + std::to_string(i);

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
            std::cout<<"1"<<std::endl;
            gridBuffers[i][j]->update_key(true);
            std::cout<<"2"<<std::endl;

            ptr_with_size tmp = gridBuffers[i][j]->attach();
            std::cout<<"2"<<std::endl;

            int width = abs(gridDims[j*6] - gridDims[j*6 + 3]) + 1;
            std::cout<<"3"<<std::endl;

            int height = abs(gridDims[j*6 + 1] - gridDims[j*6 + 4]) + 1;
            std::cout<<"4"<<std::endl;

            int depth = abs(gridDims[j*6 + 2] - gridDims[j*6 + 5]) + 1;
            std::cout<<"5"<<std::endl;

            int gridSize = width * height * depth;
            std::cout<<"6"<<std::endl;

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

void distributeVDIs(JNIEnv *e, jobject clazzObject, jobject subVDICol, jint sizePerProcess, jint commSize) {
    std::cout<<"In distribute VDIs function. Comm size is "<<commSize<<std::endl;

    void *ptrCol = e->GetDirectBufferAddress(subVDICol);
//    void *ptrDepth = e->GetDirectBufferAddress(subVDIDepth);

    void *recvBufCol = malloc(sizePerProcess * commSize * 3);
//    void *recvBufDepth = malloc(sizePerProcess * rCommSize * 2);

    MPI_Alltoall(ptrCol, sizePerProcess*3, MPI_BYTE, recvBufCol, sizePerProcess*3, MPI_BYTE, renComm);
//    MPI_Alltoall(ptrDepth, sizePerProcess*2, MPI_BYTE, recvBufDepth, sizePerProcess*2, MPI_BYTE, mpiComm);

    jclass clazz = e->GetObjectClass(clazzObject);
    jmethodID compositeMethod = e->GetMethodID(clazz, "compositeVDIs", "(Ljava/nio/ByteBuffer;I)V");

    jobject bbCol = e->NewDirectByteBuffer(recvBufCol, sizePerProcess * commSize * 3);
//    jobject bbDepth = e->NewDirectByteBuffer(recvBufDepth, sizePerProcess * rCommSize * 2);

    if(e->ExceptionOccurred()) {
        e->ExceptionDescribe();
        e->ExceptionClear();
    }

    std::cout<<"Finished distributing the VDIs. Calling the Composite method now!"<<std::endl;

    e->CallVoidMethod(clazzObject, compositeMethod, bbCol, sizePerProcess);
    if(e->ExceptionOccurred()) {
        e->ExceptionDescribe();
        e->ExceptionClear();
    }
}

void gatherCompositedVDIs(JNIEnv *e, jobject clazzObject, jobject compositedVDIColor, jint root, jint compositedVDILen, jint myRank, jint commSize, jboolean saveFiles) {

    std::cout<<"In Gather function " <<std::endl;
    void *ptrCol = e->GetDirectBufferAddress(compositedVDIColor);
//    void *ptrDepth = e->GetDirectBufferAddress(compositedVDIDepth);
//    std::cout << "Data received as parameter is: " << (char *)ptrCol << std::endl;
    void *recvBufCol;
//    void *recvBufDepth;
    if (myRank == 0) {
        recvBufCol = malloc(compositedVDILen * commSize * 3);
//        recvBufDepth = malloc(compositedVDILen * rCommSize * 2);
    }
    else {
        recvBufCol = NULL;
//        recvBufDepth = NULL;
    }

    MPI_Gather(ptrCol, compositedVDILen*3, MPI_BYTE, recvBufCol, compositedVDILen*3, MPI_BYTE, root, renComm);
//    MPI_Gather(ptrDepth, compositedVDILen*2, MPI_BYTE, recvBufDepth, compositedVDILen*2, MPI_BYTE, root, mpiComm);
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
        if(count != 0 && count % 5 == 0) {
            double average_time = (double)totaltime/count;
            avg_recent = (double)avg_recent/5.0;
            std::cout<<"Average time for the last 5 VDIs was: "<<avg_recent<<std::endl;
            avg_recent = 0.0;
            std::cout<<"Overall average so far is"<<average_time<<" and total VDIs generated so far are "<< count <<std::endl;

        }
        if(count != 0 && count % 10 == 0) {
//        if(saveFiles) {

            if(timeVDIwrite != 0) {
                int time_elapsed = t - timeVDIwrite;
                std::cout<<"Writing VDI now. Time since the last write was performed is: " << time_elapsed <<std::endl;
                timeVDIwrite = t;
            }
            std::cout<<"Writing the final gathered VDI now"<<std::endl;

            std::string filename = "size:" + std::to_string(commSize) + "Final_VDICol" + std::to_string(count) + ".raw";
            std::ofstream b_stream(filename.c_str(),
                                   std::fstream::out | std::fstream::binary);
//            std::string filenameDepth = "size:" + std::to_string(rCommSize) + "Final_VDIDepth" + std::to_string(count) + ".raw";
//            std::ofstream b_streamDepth(filenameDepth.c_str(),
//                                   std::fstream::out | std::fstream::binary);

            if (b_stream)
            {
                b_stream.write(static_cast<const char *>(recvBufCol), compositedVDILen * commSize * 3);
//                b_streamDepth.write(static_cast<const char *>(recvBufDepth), compositedVDILen * rCommSize * 2);

                if (b_stream.good()) {
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

void InVis::doRender() {
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

//void InVis::manageVisHead() {
//    JNIEnv *env;
//    jvm->GetEnv((void **)&env, JNI_VERSION_1_6);
//    jfieldID winSizeField = env->GetFieldID(clazz, "windowSize", "I");
//    env->SetIntField(obj, winSizeField, windowSize);
//
//    std::cout<<"starting thread: gather images"<<std::endl;
//    std::thread gatherImages(&InVis::receiveImages, this);
//
//    std::cout<<"starting thread: camera"<<std::endl;
//    std::thread camera(&InVis::updateCamera, this);
//
//    std::cout<<"calling function: doRender"<<std::endl;
//    doRender();
//
//    gatherImages.join();
//    camera.join();
//
//    jvm->DestroyJavaVM();
//}

void InVis::manageRenderer() {
    JNIEnv *env;
    jvm->GetEnv((void **)&env, JNI_VERSION_1_6);
    jfieldID winSizeField = env->GetFieldID(clazz, "windowSize", "I");
    env->SetIntField(obj, winSizeField, windowSize);

    jfieldID compPartnersField = env->GetFieldID(clazz, "computePartners", "I");
    env->SetIntField(obj, compPartnersField, computePartners);

    if(vtype == vis_type::grid) {
        jfieldID commSizeField = env->GetFieldID(clazz, "commSize", "I");
        env->SetIntField(obj, commSizeField, rCommSize);
    }

    int renRank;
    MPI_Comm_rank(renderComm, &renRank);

    jfieldID rankField = env->GetFieldID(clazz, "rank", "I");
    env->SetIntField(obj, rankField, renRank);

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

    if(vtype == vis_type::particles) {
        jclass RendererClass;
        if(USE_VULKAN) {
            RendererClass = env->FindClass("graphics/scenery/backends/vulkan/VulkanRenderer");  // try to find the clas
        } else {
            RendererClass = env->FindClass("graphics/scenery/backends/opengl/OpenGLRenderer");  // try to find the class
        }

        if(RendererClass == nullptr) {
            if( env->ExceptionOccurred() ) {
                env->ExceptionDescribe();
            } else {
                std::cerr << "ERROR: Renderer class not found !" <<std::endl;
                //std::exit(EXIT_FAILURE);
            }
        }

        JNINativeMethod methods[] { { (char *)"sendImage", (char *)"(Ljava/nio/ByteBuffer;)V", (void *)&sendImage } };  // mapping table

        if(env->RegisterNatives(RendererClass, methods, 1) < 0) {                        // register it
            if( env->ExceptionOccurred() ) {
                env->ExceptionDescribe();
            } else {
                std::cerr << "ERROR: Could not register natives for Renderer !" <<std::endl;
                //std::exit(EXIT_FAILURE);
            }
        } else {
            std::cout<<"Native registered for Renderer. The return value is: "<< env->RegisterNatives(RendererClass, methods, 1) <<std::endl;
        }
    }

    else {
        JNINativeMethod methods[] { { (char *)"distributeVDIs", (char *)"(Ljava/nio/ByteBuffer;II)V", (void *)&distributeVDIs },
                                    { (char *)"gatherCompositedVDIs", (char *)"(Ljava/nio/ByteBuffer;IIIIZ)V", (void *)&gatherCompositedVDIs }
        };  // mapping table

        if(env->RegisterNatives(clazz, methods, 2) < 0) {                        // register it
            if( env->ExceptionOccurred() ) {
                env->ExceptionDescribe();
            } else {
                std::cerr << "ERROR: Could not register natives for InVisVolume!" <<std::endl;
                //std::exit(EXIT_FAILURE);
            }
        } else {
            std::cout<<"Natives registered for InVisVolume. The return value is: "<< env->RegisterNatives(clazz, methods, 2) <<std::endl;
        }
    }

    if(vtype == vis_type::particles) {
        std::cout<<"starting thread: updatePosData"<<std::endl;

        std::thread updatePosData(&InVis::getMemoryPos, this);

        std::cout<<"starting thread: updatePropsData"<<std::endl;

        std::thread updatePropsData(&InVis::getMemoryProps, this);

        std::cout<<"calling function: doRender"<<std::endl;

        doRender();

        updatePosData.join();
        updatePropsData.join();
    }

    else {
        std::cout<<"starting thread: updatePosData"<<std::endl;

        std::thread updateGridData(&InVis::getGridMemory, this);

        std::cout<<"calling function: doRender"<<std::endl;

        doRender();

        std::cout<<"Finished with doRender!"<<std::endl;

        updateGridData.join();
    }


    jvm->DestroyJavaVM();
}
