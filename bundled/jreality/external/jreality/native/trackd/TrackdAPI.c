/* TrackdAPI.c */

#include <jni.h>
#include "de_jreality_devicedriver_TrackdJNI.h"

#include "trackdAPI.h"

void *tracker;
void *controller;

  /* Assuming all useful sensors for the app are on the tracker */
  JNIEXPORT jint JNICALL Java_de_jreality_devicedriver_TrackdJNI_getNumSensors
    (JNIEnv *env, jobject obj) {
    return trackdGetNumberOfSensors(tracker);
  }

  /* Assuming all buttons are on the controller */
  JNIEXPORT jint JNICALL Java_de_jreality_devicedriver_TrackdJNI_getNumButtons
    (JNIEnv *env, jobject obj) {
    return trackdGetNumberOfButtons(controller);
  }

  /* No checking is done to make sure you chose an appropriate "which" index */
  JNIEXPORT void JNICALL Java_de_jreality_devicedriver_TrackdJNI_getMatrix
    (JNIEnv *env, jobject obj, jfloatArray matrix_j, jint which) {
    float mat[4][4];
    register int i=0;
    jfloat *result = (*env)->GetFloatArrayElements(env, matrix_j, 0);
    
    trackdGetMatrix(tracker, which, mat);
    for(;i<16;i++){
      result[i] = mat[i%4][i/4];
    }
    (*env)->ReleaseFloatArrayElements(env, matrix_j, result, 0);
  }

  JNIEXPORT jint JNICALL Java_de_jreality_devicedriver_TrackdJNI_getButton
    (JNIEnv *env, jobject obj, jint which) {
    return trackdGetButton(controller, which);
  }

  JNIEXPORT jfloat JNICALL Java_de_jreality_devicedriver_TrackdJNI_getValuator
    (JNIEnv *env, jobject obj, jint which) {
    return trackdGetValuator(controller, which);
  }

  JNIEXPORT jint JNICALL Java_de_jreality_devicedriver_TrackdJNI_getNumValuators
    (JNIEnv *env, jobject obj) {
    return trackdGetNumberOfValuators(controller);
  }

/*
 * Class:     TrackdJNI
 * Method:    init_trackd_c
 * Signature: (II[I)V
 */
  JNIEXPORT void JNICALL Java_de_jreality_devicedriver_TrackdJNI_init_1trackd_1c
    (JNIEnv *env, jobject obj, jint tracker_shmkey,
     jint controller_shmkey, jintArray status_j) {

    jint *status = (*env)->GetIntArrayElements(env, status_j, 0);
    tracker = (void *) trackdInitTrackerReader(tracker_shmkey);
    controller = (void *) trackdInitControllerReader(controller_shmkey);
    status[0] = 0;
    if (tracker == NULL) status[0] += 1;
    if (controller == NULL) status[0] += 2;
    (*env)->ReleaseIntArrayElements(env, status_j, status, 0);
  }

