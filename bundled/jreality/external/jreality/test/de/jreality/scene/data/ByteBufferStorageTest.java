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

import java.nio.ByteBuffer;
import java.nio.ByteOrder;

import junit.framework.TestCase;

/**
 * 
 */
public class ByteBufferStorageTest extends TestCase
{
  public ByteBufferStorageTest()
  {
    super("ByteBufferStorage");
  }

  DataList src;
  WritableDataList dst, cmp;

  public void testDoubleArray()
  {
    double[] data={ 1, 2, 3, 42, 0.815, -123456.789 };
    setupTarget(data.length*8);
    check(StorageModel.DOUBLE_ARRAY, data);
  }
  public void testDoubleDoubleInlinedArray()
  {
    double[] data={ 1, 2, 3, 42, 0.815, -123456.789 };
    setupTarget(data.length*8);
    check(StorageModel.DOUBLE_ARRAY.inlined(3), data);
  }
  public void testDoubleDoubleArray()
  {
    double[][] data={ {1, 2, 3}, {42, 0.815, -123456.789} };
    setupTarget(data.length*8*3);
    check(StorageModel.DOUBLE_ARRAY.array(3), data);
  }
  public void testConversion()
  {
    double[][] data={ {1, 2, 3}, {42, 0.815, -123456.789} };
    src=new DoubleArrayArray.Array(data, 3);
    setupTarget(data.length*8*3);
    System.out.println(src);
    src.copyTo(dst);

    double[] inlined=new double[data.length*3];
    cmp=StorageModel.DOUBLE_ARRAY.inlined(3).createWritableDataList(inlined);
    dst.copyTo(cmp);
    System.out.println(cmp);
    assertEquals(src, cmp);
  }
  public void testIntArray()
  {
    int[] data={ 1, 2, 3, 42, 0xcafebabe, -123456 };
    setupTarget(data.length*4);
    check(StorageModel.INT_ARRAY, data);
  }
  public void testIntIntInlinedArray()
  {
    int[] data={ 1, 2, 3, 4, 2, 4, 6, 8, 3, 6, 9, 12 };
    setupTarget(data.length*4);
    check(StorageModel.INT_ARRAY.inlined(4), data);
  }
  public void testIntIntArray()
  {
    int[][] data={ {1, 5, 3, 4}, {2, 4, 6, 8}, {3, 6, 9, 12} };
    setupTarget(data.length*16);
    check(StorageModel.INT_ARRAY.array(4), data);
  }
  private void check(StorageModel sm, Object data)
  {
    src=sm.createReadOnly(data);
//    System.out.println(src);
    src.copyTo(dst);
//    System.out.println(dst);
    cmp=sm.createWritableDataList(sm.create(src.size()));
    dst.copyTo(cmp);
//    System.out.println(cmp);
    assertEquals(src, cmp);
  }
  private void setupTarget(int size)
  {
    ByteBuffer bb= ByteBuffer.allocateDirect(size)
      .order(ByteOrder.nativeOrder());
    dst=ByteBufferStorage.MODEL.createWritableDataList(bb);
  }
}
