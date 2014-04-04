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
 * An immutable array. A good JIT compiler optimizes this such that access it is not slower
 * as for the pure array. The advantage is, that only the creator who provided the array is allowed to 
 * change it.
 * 
 * @version 1.0
 * @author weissman
 *
 */
public class StringArray extends DataList {
  transient final String[] data;
  transient final int offset, length;

  public StringArray(String[] data) {
    this(data, 0, data.length);
  }
  public StringArray(String[] data, int offset, int length) {
    super(StorageModel.STRING_ARRAY, data, offset, length);
    this.data= data;
    this.offset=offset;
    this.length=length;
  }
  public StringArray toStringArray() {
    return this;
  }
  public final String[] toStringArray(String[] target) {
      if(target==null) target=new String[length];
      //TODO: why not System.arraycopy()?
      for(int src=offset, dst=0, n=length; dst<n; src++, dst++)
        target[dst]=data[src];
      return target;
    }  
  public final String getValueAt(final int n) {
    if(n>=length) throw new ArrayIndexOutOfBoundsException();
    return data[n+offset];
  }
  public final int getLength() {
    return length;
  }
}
