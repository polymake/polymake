/**
 *
 * This file is part of jReality. jReality is open source software, made
 * available under a BSD license:
 *
 * Copyright (c) 2003-2006, jReality Group: Charles Gunn, Tim Hoffmann, Markus
 * Schmies, Steffen Weissmann.
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * - Neither the name of jReality nor the names of its contributors nor the
 *   names of their associated organizations may be used to endorse or promote
 *   products derived from this software without specific prior written
 *   permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */


package de.jreality.scene.data;

import java.io.ObjectStreamException;

/**
 * Storage model using a int array.
 */
final class StringArrayStorage extends StorageModel {

  StringArrayStorage() {
    super("String[]");
  }

  public Object getAsObject(Object data, int index) {
    String[] a= (String[])data;
    return a == null ? null : a[index];
  }

  public int getLength(Object data) {
    return ((String[])data).length;
  }

  public void setAsObject(Object data, int index, Object value) {
    final String[] a= (String[])data;
    a[index]= value.toString();
  }

  public Object create(int size) {
    return new String[size];
  }

  public int getNumberOfDimensions() {
    return 1;
  }

  public int[] getDimensions(int[] dim, int d) {
    return dim;
  }

  public boolean isArray() {
    return true;
  }


  public StorageModel getComponentModel() {
    return StorageModel.ObjectType.STRING;
  }

  public void toStringImpl(Object data, int index, StringBuffer target) {
    final String[] a= (String[])data;
    target.append(a[index]);
  }

  public DataList createReadOnly(final Object data, int off, int len) {
    return new StringArray((String[])data, off, len);
  }

  void copy(Object from, int srcOff, StorageModel toFmt, Object to,
    int dstOff, int len) {
    final String[] source=(String[])from;
    if(toFmt==this) {
      final String[] target=(String[])to;
      for(int src=srcOff, dst=0; dst<len; src++, dst++)
        target[dst+dstOff]=source[src];
    } else super.copy(from, srcOff, toFmt, to, dstOff, len);
    //throw new UnsupportedOperationException("int[] => "+toFmt);
  }

  public DataItem item(Object data, int i) {
    final String[] iarray=(String[])data;
    return new DataItem(iarray, i) {
      public StorageModel getStorageModel() {
        return StorageModel.ObjectType.STRING;
      }
      public Object get(int arg0) {
        return iarray[offset];
      }
      public int size() {
        return 1;
      }
    };
  }
  /**
   * {@inheritdoc}
   */
  public StorageModel inlined(int numPerEntry) {
    if(numPerEntry<1) throw new IllegalArgumentException();
    if(inlined!=null&&inlined.length>numPerEntry&&inlined[numPerEntry]!=null)
      return inlined[numPerEntry];
    return new StorageModel.SAI(numPerEntry);
  }
  /**
   * {@inheritdoc}
   */
  public StorageModel array(int numPerEntry) {
    if(arrayof!=null&&arrayof.length>numPerEntry&&arrayof[numPerEntry]!=null)
      return arrayof[numPerEntry];
    return new StorageModel.SAA(numPerEntry);
  }
  /**
   * {@inheritdoc}
   */
  public StorageModel array() {
    StorageModel sm=arrayof!=null? arrayof[0]: null;
    return sm!=null? sm: new StorageModel.SAA();
  }
  Object readResolve() throws ObjectStreamException
  {
    return StorageModel.STRING_ARRAY;
  }
}
