/*
 * InVisRenderer.hpp
 *
 *  Created on: May 12, 2020
 *      Author: Aryaman Gupta
 */

#ifndef OPENFPM_PDATA_INVISRENDERER_HPP
#define OPENFPM_PDATA_INVISRENDERER_HPP

#include <jni.h>

#define windowSize 700
#define computePartners 2
#define imageSize windowSize*windowSize*7

class InVisRenderer
{
    JavaVM *jvm;

    void getMemoryPos(JNIEnv *env, jclass renderClass, jobject renderObject);
    void getMemoryProps(JNIEnv *env, jclass renderClass, jobject renderObject);
    void sendImage(JNIEnv *e, jobject clazz, jobject image);
    void doRender(JNIEnv *env, jclass inVisClass, jobject inVisObject);

public:
    void manageVisRenderer();
};

#endif //OPENFPM_PDATA_INVISRENDERER_HPP
