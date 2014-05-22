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
 * @version 1.0
 * @author weissman
 *
 */
public abstract class StringArrayArray extends DataList {

  private transient final int offset, length;
  private transient final StringArray[] arrays;

  StringArrayArray(StorageModel sm, Object data) {
    this(sm, data, 0, sm.getLength(data));
  }
  StringArrayArray(StorageModel sm, Object data, int first, int num) {
    super(sm, data, first, num);
    offset=first;
    length=num;
    arrays=new StringArray[length];
  }
  public abstract String getValueAt(final int n, final int i);
  public final int getLength() {
    return super.length;
  }
  public abstract int getLengthAt(final int n);
  public abstract StringArray getValueAt(final int n);
  public final StringArrayArray toStringArrayArray() {
    return this;
  }
  public String[][] toStringArrayArray(String[][] target) {
    if(target==null) target=new String[getLength()][];
    for(int i=0, n=getLength(); i<n; i++) {
      String[] slot=target[i];
      final int slotlen=getLengthAt(i);
      if(slot==null) slot=target[i]=new String[slotlen];
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
  public static final class Array extends StringArrayArray {
    private transient final String[][] data;

    public Array(final String[][] data) {
      super(StorageModel.STRING_ARRAY_ARRAY, data, 0, data.length);
      this.data= data;
    }

    public String getValueAt(int n, int i) {
      return data[n][i];
    }

    public int getLengthAt(int n) {
      return data[n].length;
    }

    public StringArray getValueAt(int n) {
      return subArray(n);
    }

    public DataItem item(int index) {
      return subArray(index);
    }

    private StringArray subArray(int index) {
      StringArray sarr=super.arrays[index];
      if(sarr!=null&&sarr.data==data[index]) return sarr;
      return super.arrays[index]=new StringArray(data[index]);
    }
  }
  public static final class Inlined extends StringArrayArray {
    private transient StringArray daView;
    private transient final String[] data;
    private transient final int entryLength;
    public Inlined(final String[] initialData, int numPerEntry) {
      this(initialData, numPerEntry, 0, initialData.length/numPerEntry);
    }
    public Inlined(final String[] initialData, int numPerEntry,
                   int firstEntry, int numEntries) {
      super(StorageModel.STRING_ARRAY.inlined(numPerEntry), initialData,
            firstEntry, numEntries);
      data=initialData;
      if(numPerEntry<1)
        throw new IllegalArgumentException("numPerEntry="+numPerEntry);
      entryLength=numPerEntry;
    }

    public StringArray toStringArray() {
      return daView!=null? daView: (daView=new StringArray(
        data, super.offset*entryLength, super.length*entryLength));
    }

    public int getLengthAt(int n)
    {
      return entryLength;
    }

    public String getValueAt(int n, int i)
    {
      return data[(n+super.offset)*entryLength+i];
    }

    public StringArray getValueAt(int n)
    {
      return subArray(n);
    }

    public DataItem item(int index)
    {
      return subArray(index);
    }

    private StringArray subArray(int ix) {
      StringArray sarr=super.arrays[ix];
      return (sarr!=null)? sarr: (super.arrays[ix]
        =new StringArray(data, (ix+super.offset)*entryLength, entryLength));
    }
  }
}