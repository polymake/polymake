/* Copyright (c) 1997-2023
   Ewgenij Gawrilow, Michael Joswig, and the polymake team
   Technische Universität Berlin, Germany
   https://polymake.org

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2, or (at your option) any
   later version: http://www.gnu.org/licenses/gpl.txt.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
--------------------------------------------------------------------------------
*/

#include "de_tuberlin_polymake_common_SharedMemoryMatrix.h"
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct PTL_Matrix {
   long refc;
   size_t size;
   long dimr, dimc;
   double elem[1];
} PTL_Matrix;

static jclass thisCls=0, psCls, pmpCls, excCls;
static jfieldID jSmmAddr, jDim, pointsArray, pmpCoords;

#define CacheID(var, expr)                                       \
   var=(*jenv)->expr;                                            \
   if (!var) return;                                             \
   var=(*jenv)->NewWeakGlobalRef(jenv, (jobject)var);            \
   if (!var) return

JNIEXPORT void JNICALL Java_de_tuberlin_polymake_common_SharedMemoryMatrix_attachToShm
   (JNIEnv *jenv, jobject this, jint key) {
   long addr;
   int err=0;

   if (!thisCls) {
      // Initialisierung der jni-Classes, methodIDs und fieldIDs

      // jclasses
      CacheID(thisCls, GetObjectClass(jenv, this));
      CacheID(psCls, FindClass( jenv, "de/tuberlin/polymake/common/geometry/PointSet"));
      CacheID(pmpCls, FindClass( jenv, "de/tuberlin/polymake/common/geometry/PolymakePoint"));
      CacheID(excCls, FindClass( jenv, "de/tuberlin/polymake/common/SharedMemoryMatrixException"));

      // fieldIDs
      jSmmAddr=(*jenv)->GetFieldID(jenv, thisCls, "addr", "J");
      jDim=(*jenv)->GetFieldID( jenv, psCls, "dim", "I");
      pointsArray=(*jenv)->GetFieldID( jenv, psCls, "points", "[Lde/tuberlin/polymake/common/geometry/PolymakePoint;");
      pmpCoords=(*jenv)->GetFieldID( jenv, pmpCls, "coords", "[D");
   }

   addr=(long)shmat(key,0,0);
   if (addr==-1L) err=errno;
   (*jenv)->SetLongField( jenv, this, jSmmAddr, addr );

   switch (err) {
   case EACCES:
      (*jenv)->ThrowNew( jenv, excCls, "access to shared memory denied" );
      break;
   case EINVAL:
      (*jenv)->ThrowNew( jenv, excCls, "invalid shared memory key" );
      break;
   default:
      (*jenv)->ThrowNew( jenv, excCls, "error in shmat");
   case 0:
      break;
   }
}


JNIEXPORT void JNICALL Java_de_tuberlin_polymake_common_SharedMemoryMatrix_finalize
  (JNIEnv *jenv, jobject this) {

   long addr;
   addr = (long)(*jenv)->GetLongField( jenv, this, jSmmAddr );
   shmdt( (void *) addr );
}


JNIEXPORT void JNICALL Java_de_tuberlin_polymake_common_SharedMemoryMatrix_copyCoords__Lde_tuberlin_polymake_common_geometry_PointSet_2
  (JNIEnv *jenv, jobject this, jobject ps) {
   PTL_Matrix *m =(PTL_Matrix*)(long)(*jenv)->GetLongField( jenv, this, jSmmAddr );
   const double *src=m->elem;
   void *dst;

   int i, dim = (*jenv)->GetIntField( jenv, ps, jDim );
   jobjectArray Points = (jobjectArray) (*jenv)->GetObjectField( jenv, ps, pointsArray );
   jobject Point;
   jdoubleArray Coords;

   if ( (*jenv)->GetArrayLength( jenv, Points ) != m->dimr || dim != m->dimc) {
      char* msg=malloc(100);
      snprintf(msg, 100, "dimension mismatch between shared matrix(%ldx%ld) and Java object Points(%dx%d)",
               m->dimr, m->dimc, (*jenv)->GetArrayLength( jenv, Points ), dim);
      (*jenv)->ThrowNew( jenv, excCls, msg );
      free(msg);
      return;
   }

   for ( i = 0; i < m->dimr; ++i, src+=dim ) {
	Point = (*jenv)->GetObjectArrayElement( jenv, Points, i );
        Coords = (jdoubleArray) (*jenv)->GetObjectField( jenv, Point, pmpCoords );
	dst = (*jenv)->GetPrimitiveArrayCritical( jenv, Coords, 0 );
	memcpy(dst, src, dim*sizeof(double));
        (*jenv)->ReleasePrimitiveArrayCritical( jenv, Coords, dst, 0 );
	(*jenv)->DeleteLocalRef( jenv, Coords );
	(*jenv)->DeleteLocalRef( jenv, Point );
   }
   (*jenv)->DeleteLocalRef( jenv, Points );
}

JNIEXPORT void JNICALL Java_de_tuberlin_polymake_common_SharedMemoryMatrix_copyCoords__ILde_tuberlin_polymake_common_geometry_PolymakePoint_2
  (JNIEnv *jenv, jobject this, jint i, jobject Point) {

   PTL_Matrix *m =(PTL_Matrix*)(long)(*jenv)->GetLongField( jenv, this, jSmmAddr );
   const double *src;
   void *dst;
   jdoubleArray Coords;

   if (i<0 || i>=m->dimr) {
      char* msg=malloc(100);
      snprintf(msg, 100, "index %d out of range [0..%ld[", i, m->dimr);
      (*jenv)->ThrowNew( jenv, excCls, msg);
      free(msg);
      return;
   }

   Coords=(jdoubleArray) (*jenv)->GetObjectField( jenv, Point, pmpCoords );
   if ((*jenv)->GetArrayLength( jenv, Coords ) != m->dimc) {
      char* msg=malloc(100);
      snprintf(msg, 100, "dimension mismatch between shared matrix(%ldx%ld) and Java object Point(%d)",
               m->dimr, m->dimc, (*jenv)->GetArrayLength( jenv, Coords ));
      (*jenv)->ThrowNew( jenv, excCls, msg);
      free(msg);
      return;
   }

   src = m->elem+i*m->dimc;
   dst = (*jenv)->GetPrimitiveArrayCritical( jenv, Coords, 0 );
   memcpy(dst, src, m->dimc*sizeof(double));
   (*jenv)->ReleasePrimitiveArrayCritical( jenv, Coords, dst, 0 );
   (*jenv)->DeleteLocalRef( jenv, Coords );
}

JNIEXPORT void JNICALL 
Java_de_tuberlin_polymake_common_SharedMemoryMatrix_setCoords__Lde_tuberlin_polymake_common_geometry_PointSet_2
  (JNIEnv *jenv, jobject this, jobject ps) {

   PTL_Matrix *m =(PTL_Matrix*)(long)(*jenv)->GetLongField( jenv, this, jSmmAddr );
   double *dst=m->elem, *src;

   int i, dim=(*jenv)->GetIntField( jenv, ps, jDim );
   jobjectArray Points = (jobjectArray) (*jenv)->GetObjectField( jenv, ps, pointsArray );
   jobject Point;
   jdoubleArray Coords;

   if ( (*jenv)->GetArrayLength( jenv, Points ) != m->dimr || dim != m->dimc) {
      char* msg=malloc(100);
      snprintf(msg, 100, "dimension mismatch between shared matrix(%ldx%ld) and Java object Points(%dx%d)",
               m->dimr, m->dimc, (*jenv)->GetArrayLength( jenv, Points ), dim);
      (*jenv)->ThrowNew( jenv, excCls, msg );
      free(msg);
      return;
   }

   for ( i = 0; i < m->dimr; ++i, dst+=dim ) {
      Point = (*jenv)->GetObjectArrayElement( jenv, Points, i );
      Coords = (jdoubleArray) (*jenv)->GetObjectField( jenv, Point, pmpCoords );
      src = (*jenv)->GetDoubleArrayElements( jenv, Coords, 0 );
      memcpy(dst, src, dim*sizeof(double));
      (*jenv)->ReleaseDoubleArrayElements(jenv, Coords, src, JNI_ABORT);
      (*jenv)->DeleteLocalRef( jenv, Coords );
      (*jenv)->DeleteLocalRef( jenv, Point );
   }
   (*jenv)->DeleteLocalRef( jenv, Points );
}

JNIEXPORT void JNICALL Java_de_tuberlin_polymake_common_SharedMemoryMatrix_setCoords__ILde_tuberlin_polymake_common_geometry_PolymakePoint_2
  (JNIEnv *jenv, jobject this, jint i, jobject Point) {

   PTL_Matrix *m =(PTL_Matrix*)(long)(*jenv)->GetLongField( jenv, this, jSmmAddr );
   double *dst, *src;
   jdoubleArray Coords;

   if (i<0 || i>=m->dimr) {
      char* msg=malloc(100);
      snprintf(msg, 100, "index %d out of range [0..%ld[", i, m->dimr);
      (*jenv)->ThrowNew( jenv, excCls, msg);
      free(msg);
      return;
   }

   Coords=(jdoubleArray) (*jenv)->GetObjectField( jenv, Point, pmpCoords );
   if ((*jenv)->GetArrayLength( jenv, Coords ) != m->dimc) {
      char* msg=malloc(100);
      snprintf(msg, 100, "dimension mismatch between shared matrix(%ldx%ld) and Java object Point(%d)",
               m->dimr, m->dimc, (*jenv)->GetArrayLength( jenv, Coords ));
      (*jenv)->ThrowNew( jenv, excCls, msg);
      free(msg);
      return;
   }

   dst = m->elem+i*m->dimc;
   src = (*jenv)->GetDoubleArrayElements( jenv, Coords, 0 );
   memcpy(dst, src, m->dimc*sizeof(double));
   (*jenv)->ReleaseDoubleArrayElements(jenv, Coords, src, JNI_ABORT);
   (*jenv)->DeleteLocalRef( jenv, Coords );
}

JNIEXPORT jint JNICALL Java_de_tuberlin_polymake_common_SharedMemoryMatrix_getNPoints
  (JNIEnv *jenv, jobject this)
{
   PTL_Matrix *m =(PTL_Matrix*)(long)(*jenv)->GetLongField( jenv, this, jSmmAddr );
   return (int) m->dimr;
}


JNIEXPORT jint JNICALL Java_de_tuberlin_polymake_common_SharedMemoryMatrix_getDim
  (JNIEnv *jenv, jobject this)
{
   PTL_Matrix *m =(PTL_Matrix*)(long)(*jenv)->GetLongField( jenv, this, jSmmAddr );
   return (int) m->dimc;
}

// Local Variables:
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
