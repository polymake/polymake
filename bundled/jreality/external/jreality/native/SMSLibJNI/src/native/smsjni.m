//
//  smsjni.m
//  FooJNI
//
//  Created by Steffen Weissmann on 03.09.09.
//  Copyright 2009 TU Berlin. All rights reserved.
//
#import "smslib.h"

//#import <JavaNativeFoundation/JavaNativeFoundation.h> // helper framework for Cocoa and JNI development

#import "de_jreality_macosx_sms_SMSLib.h" // generated from NativeAddressBook.java

/*
 * Class:     de_jreality_macosx_sms_SMSLib
 * Method:    init
 * Signature: ()V
 */
JNIEXPORT jint JNICALL Java_de_jreality_macosx_sms_SMSLib_init
(JNIEnv *env, jclass clazz) {
	jint result;
	NSAutoreleasePool *pool;
	pool = [[NSAutoreleasePool alloc] init];
	result = (jint) smsStartup(nil, nil);
	[pool release];
	printf("startup return value=%d\n", result);
	return result;
}

/*
 * Class:     de_jreality_macosx_sms_SMSLib
 * Method:    shutdown
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_de_jreality_macosx_sms_SMSLib_shutdown
(JNIEnv *env, jclass clazz) {
	printf("Shutting down SMS connection.\n");
	smsShutdown();
}
/*
 * Class:     de_jreality_macosx_sms_SMSLib
 * Method:    getValues
 * Signature: ()[F
 */
JNIEXPORT jfloatArray JNICALL Java_de_jreality_macosx_sms_SMSLib_getValues
(JNIEnv *env, jclass clazz) {
	sms_acceleration acc;
//	int ret = smsGetData(&acc);
	smsGetData(&acc);
	jfloatArray jr = (*env)->NewFloatArray(env, 3);
	jfloat *data = (jfloat *) malloc(sizeof(jfloat)*3);
	data[0] = acc.x; data[1] = acc.y; data[2] = acc.z;	
	(*env)->SetFloatArrayRegion(env,jr, 0, 3, data);
	free(data);
	return jr;	
}
