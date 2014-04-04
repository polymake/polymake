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
 * Storage model using a double array.
 */
class DoubleArrayStorage extends StorageModel
{

  DoubleArrayStorage()
  {
    super("double[]");
  }

  public Object getAsObject(Object data, int index)
  {
    double[] d=(double[])data;
    double[] r={ d[index] };
    return r; // new Double(d[index]);
  }

  public int getLength(Object data)
  {
    return ((double[])data).length;
  }

  public void setAsObject(Object data, int index, Object value)
  {
    double v;
    if(value instanceof Number) v=((Number)value).doubleValue();
    else v=((double[])value)[0];
    double[] d=(double[])data;
    d[index]=v;
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
    return StorageModel.Primitive.DOUBLE;
  }

  public Object create(int size)
  {
    return new double[size];
  }

  public DoubleArray toDoubleArray(Object data)
  {
    return new DoubleArray((double[])data);
  }

  void copy(Object from, int srcOff, StorageModel toFmt, Object to,
    int dstOff, int len) {
    final double[] source=(double[])from;
    if(toFmt==this) {
      final double[] target=(double[])to;
      for(int src=srcOff, dst=0; dst<len; src++, dst++)
        target[dst+dstOff]=source[src];
    } else super.copy(from, srcOff, toFmt, to, dstOff, len);
    //throw new UnsupportedOperationException("double[] => "+toFmt);
  }

  public void toStringImpl(Object data, int index, StringBuffer target)
  {
    double[] d=(double[])data;
    target.append(d[index]);
  }

  public DataList createReadOnly(final Object data, int off, int len) {
    return new DoubleArray((double[])data, off, len);
  }


  public DataItem item(Object data, int i) {
    final double[] darray=(double[])data;
    return new DataItem(data, i) {
      public StorageModel getStorageModel() {
        return StorageModel.Primitive.DOUBLE;
      }
      public Object get(int arg0) {
        return new Double(darray[offset]);
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
    return new StorageModel.DAI(numPerEntry);
  }
  /**
   * {@inheritdoc}
   */
  public StorageModel array(int numPerEntry) {
    if(arrayof!=null&&arrayof.length>numPerEntry&&arrayof[numPerEntry]!=null)
      return arrayof[numPerEntry];
    return new StorageModel.DAA(numPerEntry);
  }
  /**
   * {@inheritdoc}
   */
  public StorageModel array() {
    StorageModel sm=arrayof!=null? arrayof[0]: null;
    return sm!=null? sm: new StorageModel.DAA();
  }
  Object readResolve() throws ObjectStreamException
  {
    return StorageModel.DOUBLE_ARRAY;
  }

//  final static Object BUF_OUT_LOCK=new Object();
//  final static Object BUF_IN_LOCK=new Object();
//  static ByteBuffer BBUF_OUT=allocDirect(1024);
//  static DoubleBuffer DBUF_OUT=BBUF_OUT.asDoubleBuffer();
//  static ByteBuffer BBUF_IN=allocDirect(1024);
//  static DoubleBuffer DBUF_IN=BBUF_IN.asDoubleBuffer();
//
//  void exportData(ObjectOutputStream stream, DataList list)
//    throws IOException
//  {
////    System.out.println("converting double[] to byte[]");
//    final double[] da=(double[])list.data;
//    final int num=list.length;
//    synchronized(BUF_OUT_LOCK)
//    {
//      if(DBUF_OUT.capacity()<num)
//      {
//        System.out.println("reallocate export conv buffer");
//        BBUF_OUT=allocDirect(num<<3);
//        DBUF_OUT=BBUF_OUT.asDoubleBuffer();
//        ByteOrder bo=BBUF_OUT.order();
//        System.out.println("byteorder: "+bo);
//      }
//      DBUF_OUT.position(0);
//      if(COUNT++==100)
//        System.out.println("("+COUNT+") exporting "+da.length+" doubles");
//      DBUF_OUT.put(da, list.offset, num);
//      BBUF_OUT.position(0).limit(num<<3);
//      stream.writeInt(num);
//      WritableByteChannel wBCh=Channels.newChannel(stream);
//      while(BBUF_OUT.hasRemaining()) wBCh.write(BBUF_OUT);
//    }
//  }
//  static int COUNT;
//  Object importData(ObjectInputStream stream)
//    throws IOException, ClassNotFoundException
//  {
//    final int num=stream.readInt();
//    final double[] da = new double[num];
//    synchronized(BUF_IN_LOCK)
//    {
//      if(DBUF_IN.capacity()<num)
//      {
//        System.out.println("reallocate import conv buffer");
//        BBUF_IN=allocDirect(num<<3);
//        DBUF_IN=BBUF_IN.asDoubleBuffer();
//      }
//      BBUF_IN.position(0).limit(num<<3);
//      ReadableByteChannel rBCh=Channels.newChannel(stream);
//      while(BBUF_IN.hasRemaining()) rBCh.read(BBUF_IN);
//      DBUF_IN.position(0);
//      DBUF_IN.get(da);
//    }
//    return da;
//  }
//
//  private static ByteBuffer allocDirect(final int num)
//  {
//    return ByteBuffer.allocateDirect(num).order(ByteOrder.nativeOrder());
//  }
}
