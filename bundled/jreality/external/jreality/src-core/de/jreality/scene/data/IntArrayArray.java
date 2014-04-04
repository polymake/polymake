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

/**
 * An immutable array of {@link IntArray}s. A good JIT compiler optimizes this such that access it is not slower
 * as for the pure array. The advantage is, that only the creator who provided the array is allowed to 
 * change it. This class is abstract it leaves the implementation and especially the storage model 
 * to its subclasses.
 * 
 * @version 1.0
 * @author <a href="mailto:hoffmann@math.tu-berlin.de">Tim Hoffmann</a>
 *
 */
public abstract class IntArrayArray extends DataList {

  private transient final int offset, length;
  private transient final IntArray[] arrays;

  IntArrayArray(StorageModel sm, Object data) {
    this(sm, data, 0, sm.getLength(data));
  }
  IntArrayArray(StorageModel sm, Object data, int first, int num) {
    super(sm, data, first, num);
    offset=first;
    length=num;
    arrays=new IntArray[length];
  }
  public abstract int getValueAt(final int n, final int i);
  public final int getLength() {
    return super.length;
  }
  public abstract int getLengthAt(final int n);
  public abstract IntArray getValueAt(final int n);
  public final IntArrayArray toIntArrayArray() {
    return this;
  }
  public int[][] toIntArrayArray(int[][] target) {
    if(target==null) target=new int[getLength()][];
    for(int i=0, n=getLength(); i<n; i++) {
      int[] slot=target[i];
      final int slotlen=getLengthAt(i);
      if(slot==null) slot=target[i]=new int[slotlen];
      for(int j=0; j<slotlen; j++) slot[j]=getValueAt(i, j);
    }
    return target;
  }

  /**
   * This implementation of IntArrayArray stores the data as an array of 2-arrays.
   * @version 1.0
   * @author <a href="mailto:hoffmann@math.tu-berlin.de">Tim Hoffmann</a>
   *
   */
  public static final class Array extends IntArrayArray {
    private transient final int[][] data;

    public Array(final int[][] data) {
      super(StorageModel.INT_ARRAY_ARRAY, data, 0, data.length);
      this.data= data;
    }
    
    public Array(final int[][] data, int numPerEntry) {
      super(StorageModel.INT_ARRAY.array(numPerEntry), data);
      this.data=data;
    }

    public int getValueAt(int n, int i) {
      return data[n][i];
    }

    public int getLengthAt(int n) {
      return data[n].length;
    }

    public IntArray getValueAt(int n) {
      return subArray(n);
    }

    public DataItem item(int index) {
      return subArray(index);
    }

    private IntArray subArray(int index) {
      IntArray sarr=super.arrays[index];
      if(sarr!=null&&sarr.data==data[index]) return sarr;
      return super.arrays[index]=new IntArray(data[index]);
    }
  }
  public static final class Inlined extends IntArrayArray {
    private transient IntArray daView;
    private transient final int[] data;
    private transient final int entryLength;
    public Inlined(final int[] initialData, int numPerEntry) {
      this(initialData, numPerEntry, 0, initialData.length/numPerEntry);
    }
    public Inlined(final int[] initialData, int numPerEntry,
                   int firstEntry, int numEntries) {
      super(StorageModel.INT_ARRAY.inlined(numPerEntry), initialData,
            firstEntry, numEntries);
      data=initialData;
      if(numPerEntry<1)
        throw new IllegalArgumentException("numPerEntry="+numPerEntry);
      entryLength=numPerEntry;
    }

    public IntArray toIntArray() {
      return daView!=null? daView: (daView=new IntArray(
        data, super.offset*entryLength, super.length*entryLength));
    }

    public int getLengthAt(int n)
    {
      return entryLength;
    }

    public int getValueAt(int n, int i)
    {
      return data[(n+super.offset)*entryLength+i];
    }

    public IntArray getValueAt(int n)
    {
      return subArray(n);
    }

    public DataItem item(int index)
    {
      return subArray(index);
    }

    private IntArray subArray(int ix) {
      IntArray sarr=super.arrays[ix];
      return (sarr!=null)? sarr: (super.arrays[ix]
        =new IntArray(data, (ix+super.offset)*entryLength, entryLength));
    }
  }
}