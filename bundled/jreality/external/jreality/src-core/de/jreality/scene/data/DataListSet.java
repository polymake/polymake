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
import java.io.Serializable;
import java.util.Collections;
import java.util.HashMap;
import java.util.Map;
import java.util.Set;

/**
 * @author Holger
 */
public class DataListSet implements Serializable {
  private int dataSize;
  protected Map<Attribute, DataList> attr2attrDataList=new HashMap<Attribute, DataList>();
  protected transient Set<Attribute> keySet=Collections.unmodifiableSet(attr2attrDataList.keySet());
  /**
   * 
   */
  public DataListSet(int numDataItems) {
    if(numDataItems<0)
      throw new IllegalArgumentException("numDataItems "+numDataItems);
    dataSize=numDataItems;
  }

  /**
   * Clears the map and set the list length.
   */
  public void reset(int numDataItems) {
    if(numDataItems<0)
      throw new IllegalArgumentException("numDataItems "+numDataItems);
    attr2attrDataList.clear();
    dataSize=numDataItems;
  }

  public void remove(Attribute a)	{
   	attr2attrDataList.remove(a);
  }
  
//  public DataList add(Attribute a, Object v) {
//    DataList dList=a.getDefaultStorageModel()
//                    .createWritableDataList(v).readOnlyList();
//    attr2attrDataList.put(a, dList);
//    return dList;
//  }
//  public WritableDataList addWritable(Attribute a) {
//    return addWritable(a, a.getDefaultStorageModel());
//  }
  public WritableDataList addWritable(Attribute a, StorageModel sm) {
    return addWritable(a, sm, sm.create(dataSize));
  }

  public WritableDataList addWritable(
    Attribute a, StorageModel sm, Object data) {
    WritableDataList wList= sm.createWritableDataList(data);
    attr2attrDataList.put(a, wList);
    return wList;
  }
  public void addReadOnly(Attribute a, StorageModel sm, Object data) {
    if (sm.getLength(data) != dataSize)
      throw new IllegalArgumentException("incompatible sizes");
    attr2attrDataList.put(a, sm.createReadOnly(data));
  }
  public boolean containsAttribute(Attribute attr ) {
	  return attr2attrDataList.containsKey(attr);
  }
  public DataList getList(Attribute attr) {
    DataList list= (DataList)attr2attrDataList.get(attr);
    return list!=null? list.readOnlyList(): null;
  }
  public WritableDataList getWritableList(Attribute attr) {
    try {
      return (WritableDataList)attr2attrDataList.get(attr);
    } catch (ClassCastException ex) {
      throw new IllegalStateException("readOnly");
    }
  }
  public DataItem get(Attribute attr, int index) {
    return getList(attr).item(index);
  }
  public Object set(Attribute attr, int index, Object value) {
    return getWritableList(attr).set(index, value);
  }
  public String toString() {
    return attr2attrDataList.toString();
  }

  public int getListLength() {
    return dataSize;
  }

  public int getNumAttributes() {
    return attr2attrDataList.size();
  }

  public Set<Attribute> storedAttributes() {
    return keySet;
  }

  public DataListSet readOnly()
  {
    return new RO(this);
  }

  static final class RO extends DataListSet
  {
    final DataListSet source;
    RO(DataListSet src)
    {
      super(src.getListLength());
      source=src;
      super.attr2attrDataList=Collections.unmodifiableMap(src.attr2attrDataList);
      super.keySet=src.keySet;
    }
    public int getListLength()
    {
      return source.getListLength();
    }
    public WritableDataList getWritableList(Attribute attr) {
      throw new IllegalStateException("readOnly");
    }
    public WritableDataList
    addWritable(Attribute a, StorageModel sm, Object data) {
      throw new IllegalStateException("readOnly");
    }
    public DataListSet readOnly()
    {
      return this;
    }
  }
  private void readObject(ObjectInputStream is)
    throws IOException, ClassNotFoundException {
      is.defaultReadObject();
      keySet=Collections.unmodifiableSet(attr2attrDataList.keySet());
  }
}
