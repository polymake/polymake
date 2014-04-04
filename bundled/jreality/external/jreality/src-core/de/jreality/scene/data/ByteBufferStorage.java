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

import java.io.IOException;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.DoubleBuffer;
import java.nio.IntBuffer;
import java.nio.channels.Channels;
import java.nio.channels.ReadableByteChannel;
import java.nio.channels.WritableByteChannel;

/**
 * Storage Model for {@link ByteBufferList} which uses a
 * {@link ByteBuffer} as backend.
 */
public class ByteBufferStorage extends StorageModel
{
  public static final ByteBufferStorage MODEL = new ByteBufferStorage();

  ByteBufferStorage()
  {
    super("<buf>");
  }

  Object create(int size)
  {
    return ByteBuffer.allocateDirect(size).order(ByteOrder.nativeOrder());
  }

  public DataList createReadOnly(Object v, int start, int length)
  {
    return new ByteBufferList((ByteBuffer)v, start, length);
  }


  void copy(StorageModel fromFmt,
    Object from, int srcOff, Object to, int dstOff, int len)
  {
//      System.out.println("Copy from: fromFmt: "+fromFmt+" \nfrom: "+from+" \nsrcOff="+srcOff+" \nto="+to+" \ndstOff+"+dstOff+" \nlen="+len+"\n");
    if(fromFmt==DOUBLE_ARRAY)
      ((DoubleBuffer)((ByteBuffer)to).asDoubleBuffer().position(dstOff))
        .put((double[])from, srcOff, len);
    else if(fromFmt==INT_ARRAY)
      ((IntBuffer)((ByteBuffer)to).asIntBuffer().position(dstOff))
        .put((int[])from, srcOff, len);
    else if(fromFmt instanceof StorageModel.ArrayOf)
    {
      Object[] array=(Object[])from;
      int sLen=((StorageModel.ArrayOf)fromFmt).numPerEntry;
      if(sLen==-1)
        throw new UnsupportedOperationException("inline dynamic size");
      StorageModel csm=fromFmt.getComponentModel();
      for(int ix=0, idOff=dstOff*sLen, alen=array.length; ix < alen; ix++)
      {
        copy(csm, array[ix], 0, to, idOff, sLen);
        idOff+=sLen;
      }
    }
    else if(fromFmt instanceof StorageModel.InlinedArray)
    {
      int sLen=((StorageModel.InlinedArray)fromFmt).numPerEntry;
      StorageModel csm=fromFmt.getComponentModel();
      copy(csm, from, srcOff*sLen, to, dstOff, len*sLen);
    }
    else super.copy(fromFmt, from, srcOff, to, dstOff, len);
  }

  /*
   * problem here is that  
   */
  void copy(
    Object from, int srcOff, StorageModel toFmt, Object to, int dstOff, int len)
  {
//      System.out.println("Copy to: from: "+from+" \nsrcOff="+srcOff+" \ntoFmt="+toFmt+" \nto="+to+" \ndstOff+"+dstOff+" \nlen="+len+"\n");
    if(toFmt==DOUBLE_ARRAY)
      ((DoubleBuffer)
        ((ByteBuffer)from).asDoubleBuffer().position(srcOff)/*.limit(len>>3)*/)
        .get((double[])to, dstOff, len>>3);
    else if(toFmt==INT_ARRAY)
      ((IntBuffer)((ByteBuffer)from).asIntBuffer().position(srcOff)/*.limit(len>>2)*/)
        .get((int[])to, dstOff, len>>2);
    else if(toFmt instanceof StorageModel.InlinedArray)
    {
      int sLen=((StorageModel.InlinedArray)toFmt).numPerEntry;
      StorageModel csm=toFmt.getComponentModel();
      copy(from, srcOff, csm, to, dstOff*sLen, len);
    }
    else if(toFmt instanceof StorageModel.ArrayOf)
    {//XXX extracting array not fully implemented
     // possibly works only with 2-dim arrays!! 
        int factor = 1;
        if (toFmt instanceof StorageModel.DAA) factor = 8;
        if (toFmt instanceof StorageModel.IAA) factor = 4;
      Object[] array=(Object[])to;
      int sLen=((StorageModel.ArrayOf)toFmt).numPerEntry;
      if(sLen==-1)
        throw new UnsupportedOperationException("extract dynamic size");
      StorageModel csm=toFmt.getComponentModel();
      for(int ix=0, idOff=srcOff*sLen, alen=array.length; ix < alen; ix++)
      {
        Object sub=array[ix];
        if(sub==null) sub=array[ix]=csm.create(sLen);
        copy(from, idOff, csm, sub, 0, sLen*factor);
        idOff+=sLen;
      }
    }
    else {
        System.err.println("Warning: unhandled format: "+toFmt);
        super.copy(from, srcOff, toFmt, to, dstOff, len);
    }
  }

  static int numBytes(StorageModel fromFmt, Object from, int len)
  {
    if(len==0) return 0;
    if(fromFmt==DOUBLE_ARRAY)   return len<<3;
    else if(fromFmt==INT_ARRAY) return len<<2;
    else if(fromFmt instanceof StorageModel.ArrayOf)
    {
      Object[] array=(Object[])from;
      int sLen=((StorageModel.ArrayOf)fromFmt).numPerEntry;
      if(sLen==-1)
        throw new UnsupportedOperationException("inline dynamic size");
      StorageModel csm=fromFmt.getComponentModel();
      return numBytes(csm, array[0], sLen)*fromFmt.getLength(array);
    }
    else if(fromFmt instanceof StorageModel.InlinedArray)
    {
      int sLen=((StorageModel.InlinedArray)fromFmt).numPerEntry;
      StorageModel csm=fromFmt.getComponentModel();
      return numBytes(csm, from, sLen)*fromFmt.getLength(from);
    }
    throw new IllegalStateException("unknown data format "+fromFmt);
  }

  Object getAsObject(Object data, int index)
  {
    return new Byte(((ByteBuffer)data).get(index));
  }

  public int getLength(Object data)
  {
    return ((ByteBuffer)data).remaining();
  }

  public DataItem item(Object data, int i)
  {
    final ByteBuffer darray=(ByteBuffer)data;
    return new DataItem(data, i) {
      public StorageModel getStorageModel() {
        return StorageModel.Primitive.DOUBLE;
      }
      public Object get(int arg0) {
        return new Byte(darray.get(offset));
      }
      public int size() {
        return 1;
      }
    };
  }

  void exportData(ObjectOutputStream stream, DataList list) throws IOException
  {
    ByteBuffer bb=(ByteBuffer)list.data;
    // XXX something is wrong here...
    bb.position(list.offset).limit(list.length);
    stream.writeInt(list.length);
    WritableByteChannel ch=Channels.newChannel(stream);
    while(bb.hasRemaining()) ch.write(bb);
    bb.position(list.offset).limit(list.length);
  }

  Object importData(ObjectInputStream stream)
    throws IOException, ClassNotFoundException
  {
    int length=stream.readInt();
    ByteBuffer bb=(ByteBuffer)ByteBufferList.BufferPool.getBuffer(length);
    bb.position(0).limit(length);
    ReadableByteChannel ch= Channels.newChannel(stream);
    while(bb.hasRemaining()) ch.read(bb);
    bb.position(0).limit(length);
    return bb;
  }

}
