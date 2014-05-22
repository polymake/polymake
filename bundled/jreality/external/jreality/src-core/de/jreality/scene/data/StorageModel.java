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
import java.io.ObjectStreamException;
import java.io.Serializable;
import java.lang.reflect.Array;

/**
 * Representation of the format of data encapsulated by a
 * {@link DataItem} or {@link DataList}.
 * @author Holger
 */
public abstract class StorageModel implements Serializable {
  static Primitive[] PRIMITIVES=new Primitive[8];
  static ObjectType[] OBJECT_TYPES=new ObjectType[1];

  /**
   * Get the single-item storage model for a primitive data
   * type. The data type's class reference can be specified
   * using class literal syntax <code><i>type</i>.class</code>
   * e.g. <code>int.class</code> or referring to the static
   * constants of their counterpart reference type classes,
   * e.g. <code>{@link Integer#TYPE Integer.TYPE}</code>.
   * @param cl the primitive type's class representation
   * @return the shared single-item storage model
   */
  public static StorageModel primitive(Class cl) {
    if(!cl.isPrimitive()) throw new IllegalArgumentException();
    int ix;
    switch(cl.getName().charAt(0)) {
      case 'b': ix=cl==Boolean.TYPE? 0: 1; break;
      case 's': ix=2; break;
      case 'i': ix=3; break;
      case 'l': ix=4; break;
      case 'f': ix=5; break;
      case 'd': ix=6; break;
      default: case 'c': ix=7; break;
    }
    StorageModel sm=PRIMITIVES[ix];
    return sm!=null? sm: (PRIMITIVES[ix]=new Primitive(cl));
  }
  
  public static StorageModel objectType(Class cl) {
    if(cl.isPrimitive()) throw new IllegalArgumentException();
    int ix=-1;
    if (cl == String.class) ix = 0;
    if (ix == -1) throw new IllegalArgumentException("unsupported type: "+cl.getName());
    StorageModel sm=OBJECT_TYPES[ix];
    return sm!=null? sm: (OBJECT_TYPES[ix]=new ObjectType(cl));
  }

  transient StorageModel[] inlined, arrayof;
  /**
   * Create a storage model that inlines an additional dimension
   * in a linear array of length <code>*numPerEntry</code>.
   * Can be applied several times, e.g.<code>
   * StorageModel.DOUBLE_ARRAY.inline(4).inline(4)</code> creates
   * a storage model for a list of 4x4 matrices still stored in
   * a linear <code>double[]</code> array. Can also be used
   * in combination with {@link #array(int)}.
   */
  public StorageModel inlined(int numPerEntry) {
    if(numPerEntry<1) throw new IllegalArgumentException();
    if(inlined!=null&&inlined.length>numPerEntry&&inlined[numPerEntry]!=null)
      return inlined[numPerEntry];
    return new InlinedArray(this, numPerEntry);
  }
  /**
   * Create a storage model that adds an additional dimension
   * to an array specifying a constant length for each entry.
   * Can be applied several times, e.g.<code>
   * StorageModel.INT_ARRAY.inline(2).inline(3)</code> creates
   * a storage model for a list of <code>int[2][3]</code> entries.
   * Can also used combined with {@link #inlined(int)}.
   * @see #array()
   */
  public StorageModel array(int numPerEntry) {
    if(arrayof!=null&&arrayof.length>numPerEntry&&arrayof[numPerEntry]!=null)
      return arrayof[numPerEntry];
    return new ArrayOf(this, numPerEntry);
  }
  /**
   * Like {@link #array(int)} but without specifying an entry size.
   * Implies that entries might have different lengths. The resulting
   * storage model will return -1 if queried for the general entry
   * size.
   */
  public StorageModel array() {
    StorageModel sm=arrayof!=null? arrayof[0]: null;
    return sm!=null? sm: new ArrayOf(this);
  }
  public static final StorageModel INT_ARRAY
                                   = new IntArrayStorage();
  public static final StorageModel DOUBLE_ARRAY
                                   = new DoubleArrayStorage();
  public static final StorageModel INT_ARRAY_ARRAY
                                   = INT_ARRAY.array();
  public static final StorageModel DOUBLE_ARRAY_ARRAY
                                   = DOUBLE_ARRAY.array();
  public static final StorageModel DOUBLE2_INLINED
                                   = DOUBLE_ARRAY.inlined(2);
  public static final StorageModel DOUBLE3_INLINED
                                   = DOUBLE_ARRAY.inlined(3);
  public static final StorageModel DOUBLE3_ARRAY
                                   = DOUBLE_ARRAY.array(3);
  public static final StorageModel STRING_ARRAY
                                   = new StringArrayStorage();
  public static final StorageModel STRING_ARRAY_ARRAY
                                   = STRING_ARRAY.array();
  private transient String string;
  StorageModel(String str) { string=str; }
  abstract Object create(int size);
  abstract Object getAsObject(Object data, int index);

  Object copy(DataList from, StorageModel toFmt, Object to) {
    final Object data=from.data;
    final int off=from.offset, len=from.length;
    if(to==null) to=toFmt.create(linearSize(toFmt, data, off, len));
    copy(data, off, toFmt, to, 0, len);
    return to;
  }

  private int linearSize(StorageModel toFmt, Object data, int off, int len) {
    int size=len;
    int[] dim= getDimensions();
    final int toN= toFmt.getNumberOfDimensions(), n= dim.length;
    StorageModel sm=this;
    if(toN<n) {
      for(int d=1, k=0, nd=n-toN; k<nd; d++, k++) size*=dim[d];
      if(size>=0) return size;
      sm=sm.getComponentModel();
      int toInline=dim[1];
      final boolean inlined=(toInline>0&&(this instanceof InlinedArray));
      if(inlined) { off*=toInline; len*=toInline; }
      toInline=0;
      for(int j=off; j<len; j++) {
        Object o=inlined? data: getAsObject(data, j);
        toInline+=sm.linearSize(toFmt, o, 0, sm.getLength(o)); 
      }
      size=toInline;
    }
    return size;
  }
  void copy(Object from, int srcOff, StorageModel toFmt, Object to, int dstOff, int len) {
    if(toFmt==this)
      System.arraycopy(from, srcOff, to, dstOff, len);
    else
      toFmt.copy(this, from, srcOff, to, dstOff, len);
  }
  void copy(StorageModel fromFmt, Object from, int srcOff, Object to, int dstOff, int len) {
    throw new UnsupportedOperationException(fromFmt+" -> "+this);
  }

  /**
   * Like {@link createReadOnly(Object,int,int)} assuming
   * offset <code>0</code> and the entire array length.
   */
  public final DataList createReadOnly(Object v) {
    return createReadOnly(v, 0, getLength(v));
  }
  /**
   * Create a {@link DataList data list} using this data model.
   * Certain storage models will return specialized data lists,
   * e.g. {@link DoubleArray} or {@link IntArray}.
   * @param v the object to wrap
   * @param start start offset inside v
   * @param length the number of items
   */
  public DataList createReadOnly(Object v, int start, int length) {
    //System.out.println("creating list for "+this);
    return new DataList(this, v, start, length);
  }
  public WritableDataList createWritableDataList(Object v) {
    getLength(v); //ensure correct input type
    return new WritableDataList(this, v);
  }
  void toStringImpl(Object data, int index, StringBuffer target) {
    target.append(getAsObject(data, index));
  }
  /**
   * Return a string representation of the storage model, e.g. int[][3].
   */
  public String toString() {
    if(string==null) {
      final int[] dim=getDimensions();
      final int nd=dim.length;
      StorageModel sm=this;
      for(int i=0; i<nd; i++) sm=sm.getComponentModel();
      StringBuffer sb=new StringBuffer(nd*4+8);
      sb.append(sm.string);
      for(int i=0; i<nd; i++) {
        sb.append('[');
        if(dim[i]!=-1) sb.append(dim[i]);
        sb.append(']');
      }
      string=sb.toString();
    }
    return string;
  }

  /**
   * Return the length of a data item in the format of this
   * storage model.
   */
  public abstract int getLength(Object data);

  public int getNumberOfDimensions() {
    return 0;
  }
  /**
   * Get the dimension of the data. The first entry is <code>-1</code>
   * for list/array types as the is the length of the list.
   * @see #getDimensions(DataList)
   */
  public int[] getDimensions() {
    int[] dim=new int[getNumberOfDimensions()];
    dim[0]=-1;
    return getDimensions(dim, 1);
  }
  int[] getDimensions(int[] dim, int d) {
    throw new UnsupportedOperationException("dim of primitive");
  }
  /**
   * Like {@link getDimensions()} but with initialization of the
   * first entry with the length of the specified list.
   */
  public int[] getDimensions(DataList from) {
    if(from.getStorageModel()!=this)
      throw new IllegalArgumentException("list not of that type");
    int[] dim=getDimensions();
    dim[0]=getLength(from.data);
    return dim;
  }

  public boolean isArray() {
    return false;
  }
  public StorageModel getComponentModel() {
    throw new UnsupportedOperationException("component model of primitive");
  }

  static final class Primitive extends StorageModel {
    static final Primitive DOUBLE = new Primitive(Double.TYPE);
    static final Primitive INT = new Primitive(Integer.TYPE);
    final Class primitiveType;

    public Primitive(Class type) {
      super(type.getName());
      primitiveType=type;
    }

    Object create(int size) {
      throw new UnsupportedOperationException();
    }

    Object getAsObject(Object data, int index) {
      if(data!=null&&data.getClass().isArray())
        return Array.get(data, 0);
      return data;
    }

    public int getLength(Object data) {
      return 1;
    }

    public DataItem item(Object data, int i) {
      // TODO Auto-generated method stub
      return null;
    }
    private Object readResolve() throws ObjectStreamException
    {
      return primitive(primitiveType);
    }
  }

  static final class ObjectType extends StorageModel {
    static final ObjectType STRING = new ObjectType(String.class);

    final Class objectType;

    public ObjectType(Class type) {
      super(type.getName());
      objectType=type;
    }

    Object create(int size) {
      throw new UnsupportedOperationException();
    }

    Object getAsObject(Object data, int index) {
      if(data!=null&&data.getClass().isArray())
        return Array.get(data, 0);
      return data;
    }

    public int getLength(Object data) {
      return 1;
    }

    public DataItem item(Object data, int i) {
      // TODO Auto-generated method stub
      return null;
    }
    private Object readResolve() throws ObjectStreamException
    {
      return objectType(objectType);
    }
  }

  public IntArray getAsIntArray(Object data, int index) {
    try {
      return new IntArray((int[])getAsObject(data, index));
    } catch (ClassCastException ex) {
      throw new UnsupportedOperationException();
    }
  }
  public DoubleArray getAsDoubleArray(Object data, int index) {
    try {
      return new DoubleArray((double[])getAsObject(data, index));
    } catch (ClassCastException ex) {
      throw new UnsupportedOperationException();
    }
  }
  public StringArray getAsStringArray(Object data, int index) {
    try {
      return new StringArray((String[])getAsObject(data, index));
    } catch (ClassCastException ex) {
      throw new UnsupportedOperationException();
    }
  }
  public IntArray toIntArray(Object data) {
    if (data instanceof IntArray)
      return (IntArray)data;
    try {
      return new IntArray((int[])data);
    } catch (ClassCastException ex) {
      throw new UnsupportedOperationException();
    }
  }
  public IntArrayArray toIntArrayArray(Object data) {
    if (data instanceof IntArrayArray)
      return (IntArrayArray)data;
    try {
      return new IntArrayArray.Array((int[][])data);
    } catch (ClassCastException ex) {
      throw new UnsupportedOperationException();
    }
  }
  public DoubleArray toDoubleArray(Object data) {
    if (data instanceof DoubleArray)
      return (DoubleArray)data;
    try {
      return new DoubleArray((double[])data);
    } catch (ClassCastException ex) {
      throw new UnsupportedOperationException();
    }
  }
  public DoubleArrayArray toDoubleArrayArray(Object data) {
    if (data instanceof DoubleArrayArray)
      return (DoubleArrayArray)data;
    try {
      return new DoubleArrayArray.Array((double[][])data);
    } catch (ClassCastException ex) {
      throw new UnsupportedOperationException();
    }
  }
  public StringArray toStringArray(Object data) {
    if (data instanceof StringArray)
      return (StringArray)data;
    try {
      return new StringArray((String[])data);
    } catch (ClassCastException ex) {
      throw new UnsupportedOperationException();
    }
  }
  public StringArrayArray toStringArrayArray(Object data) {
    if (data instanceof StringArrayArray)
      return (StringArrayArray)data;
    try {
      return new StringArrayArray.Array((String[][])data);
    } catch (ClassCastException ex) {
      throw new UnsupportedOperationException();
    }
  }
  static class InlinedArray extends StorageModel {
    final StorageModel component;
    final int numPerEntry;
    InlinedArray(StorageModel compModel, int num) {
      super(null);
      if(compModel==null) throw new NullPointerException();
      component=compModel;
      numPerEntry=num;
      if(compModel.inlined!=null)
      {
        if(compModel.inlined.length<num){
            int l = compModel.inlined.length;
          System.arraycopy(compModel.inlined, 0, compModel.inlined
            =new StorageModel[num+32], 0, l);
        }
        else if(compModel.inlined[num]!=null)
          throw new IllegalStateException("redefinition of"+this);
      } else compModel.inlined=new StorageModel[num+32];
      compModel.inlined[num]=this;
    }

    public Object create(int size) {
      return component.create(size*numPerEntry);
    }

    public int getNumberOfDimensions() {
      return component.getNumberOfDimensions()+1;
    }
    int[] getDimensions(int[] dim, int d) {
      component.getDimensions(dim, d+1)[d]=numPerEntry;
      return dim;
    }

    void copy(Object from, int srcOff, StorageModel toFmt, Object to, int dstOff, int len) {
      if(toFmt.getNumberOfDimensions()<getNumberOfDimensions()) {
        component.copy(from, srcOff*numPerEntry, toFmt, to, dstOff*numPerEntry, len*numPerEntry);
      }
      else if(toFmt instanceof ArrayOf) {
        toFmt=((ArrayOf)toFmt).component;
        Object[] target=(Object[])to;
        for(int src=srcOff*numPerEntry, dst=0; dst<len; src+=numPerEntry, dst++)
        {
          Object t=target[dst+dstOff];
          if(t==null) t=target[dst+dstOff]=toFmt.create(numPerEntry);
          component.copy(from, src, toFmt, t, 0, numPerEntry);
        }
      } 
      else {
        if(toFmt.getNumberOfDimensions()>=getNumberOfDimensions())//while ?
          toFmt=((InlinedArray)toFmt).component;
        component.copy(from, srcOff*numPerEntry, toFmt, to, dstOff*numPerEntry, len*numPerEntry);
      }
    }

    Object getAsObject(Object data, int index) {
      Object obj=component.create(numPerEntry);
      System.arraycopy(data, index*numPerEntry, obj, 0, numPerEntry);
      return obj;
    }

    public boolean isArray() {
      return true;
    }
    public StorageModel getComponentModel() {
      return component;
    }
    public int getLength(Object data) {
      return component.getLength(data)/numPerEntry;
    }
    public DataList createReadOnly(final Object v, int start, int length) {
      if(component==INT_ARRAY) {
        final int[] value=(int[])v;
        return new IntArrayArray.Inlined(value, numPerEntry, start, length);
      }
      return new DataList(this, v, start, length);
    }
    public DataItem item(Object v, int index) {
      return component.createReadOnly(v, (index)*numPerEntry, numPerEntry);
    }
    void toStringImpl(Object data, int index, StringBuffer target) {
      index*=numPerEntry;
      target.append("{ ");
      component.toStringImpl(data, index, target);
      for(int ix=1; ix<numPerEntry; ix++) {
        target.append(", ");
        component.toStringImpl(data, index+ix, target);
      }
      target.append(" }");
    }
    //must be accessible by subclasses
    /*package-private*/ Object readResolve() throws ObjectStreamException
    {
      return component.inlined(numPerEntry);
    }
    void exportData(ObjectOutputStream out, DataList dataList)
      throws IOException
    {//TODO implement specialized exportData for IntArray
      component.exportData(out, dataList);
    }
    Object importData(ObjectInputStream in)
      throws IOException, ClassNotFoundException
    {
      return component.importData(in);
    }
  }
  static class ArrayOf extends StorageModel {
    final StorageModel component;
    final int numPerEntry;
    ArrayOf(StorageModel compModel, int num) {
      super(null);
      if(compModel==null) throw new NullPointerException();
      component=compModel;
      numPerEntry=num;
      if(compModel.arrayof!=null)
      {
        if(compModel.arrayof.length<num)
          System.arraycopy(compModel.arrayof, 0, compModel.arrayof
            =new StorageModel[num+32], 0, compModel.arrayof.length);
        else if(compModel.arrayof[num]!=null)
          throw new IllegalStateException("redefinition of"+this);
      } else compModel.arrayof=new StorageModel[num+32];
      compModel.arrayof[num]=this;
    }
    ArrayOf(StorageModel compModel) {//non-constant
      super(null);
      if(compModel==null) throw new NullPointerException();
      component=compModel;
      numPerEntry=-1;
      if(compModel.arrayof!=null)
      {
        if(compModel.arrayof[0]!=null)
          throw new IllegalStateException("redefinition of"+this);
      } else compModel.arrayof=new StorageModel[32];
      compModel.arrayof[0]=this;
     }

    public Object create(int size) {
      Object data=Array.newInstance(component.create(1).getClass(), size);
      if(numPerEntry>0&&!(component instanceof Primitive)) {
        Object[] array=(Object[])data;
        for(int i=0; i<size; i++)
          array[i]=component.create(numPerEntry);
      }
      return data;
    }

    Object getAsObject(Object data, int index) {
      return ((Object[])data)[index];
    }

    public int getLength(Object data) {
      return ((Object[])data).length;
    }

    public boolean isArray() {
      return true;
    }
    public StorageModel getComponentModel() {
      return component;
    }

    void copy(Object from, int srcOff, StorageModel toFmt, Object to, int dstOff, int len) {
      final Object[] source=(Object[])from;
      if(toFmt.getNumberOfDimensions()<getNumberOfDimensions()) {
        if(numPerEntry!=-1) dstOff*=numPerEntry;
        else {
          int toOff=0;
          for(int i=0; i<dstOff; i++) toOff+=component.getLength(source[i]);
          dstOff=toOff;
        }
        for(int i=srcOff, j=dstOff; i<len; i++) {
          int dl=numPerEntry!=-1? numPerEntry: component.getLength(source[i]);
          component.copy(source[i], 0, toFmt, to, j, dl);
          j+=dl;
        }
      }
      else if(toFmt instanceof ArrayOf) {
        toFmt=((ArrayOf)toFmt).component;
        Object[] target=(Object[])to;
        for(int src=srcOff, dst=0; dst<len; src++, dst++)
        {
          final int n=numPerEntry!=-1? numPerEntry: component.getLength(source[src]);
          Object t=target[dst+dstOff];
          if(t==null) t=target[dst+dstOff]=toFmt.create(n);
          component.copy(source[src], 0, toFmt, t, 0, n);
        }
      } 
      else {
        if(toFmt.getNumberOfDimensions()>=getNumberOfDimensions())
          toFmt=((InlinedArray)toFmt).component;
        if(numPerEntry>0)
          for(int src=0, dst=dstOff*numPerEntry; src<len; src++, dst+=numPerEntry)
            component.copy(source[src+srcOff], 0, toFmt, to, dst, numPerEntry);
        else {
          int dst=0;
          for(int i=0; i<dstOff; i++) dst+=component.getLength(source[i]);
          for(int src=srcOff, i=0; i<len; src++, i++) {
            int n=component.getLength(source[src]);
            component.copy(source[src], 0, toFmt, to, dst, n);
            dst+=n;
          }
        }
      }
    }

    public int getNumberOfDimensions() {
      return component.getNumberOfDimensions()+1;
    }
    int[] getDimensions(int[] dim, int d) {
      component.getDimensions(dim, d+1)[d]=numPerEntry;
      return dim;
    }
    void toStringImpl(Object data, int index, StringBuffer target) {
      target.append("{ ");
      data=getAsObject(data, index);
      if(data!=null) {
        int num=(numPerEntry==-1)? component.getLength(data): numPerEntry;
        if(num>0) component.toStringImpl(data, 0, target);
        for(int ix=1; ix<num; ix++) {
          target.append(", ");
          component.toStringImpl(data, ix, target);
        }
      }
      target.append(" }");
    }
    public DataItem item(Object v, int index) {
      Object[] array=(Object[])v;
      return component.createReadOnly(array[index]);
    }
    //must be accessible by subclasses
    /*package-private*/ Object readResolve() throws ObjectStreamException
    {
      return numPerEntry>0? component.array(numPerEntry): component.array();
    }
  }
  static class IAA extends ArrayOf {
    IAA() {
      super(INT_ARRAY);
    }
    IAA(int num) {
      super(INT_ARRAY, num);
    }
    public DataList createReadOnly(Object v, int start, int length) {
      final int[][] value= (int[][])v;
      if(start!=0||length!=value.length)
        throw new UnsupportedOperationException("["+start+", "+(start+length)+'[');
      return new IntArrayArray.Array(value);
//      return super.numPerEntry==-1?
//        new IntArrayArray.Array(value):
//        new IntArrayArray.Array(value, super.numPerEntry);
    }
  }
  static class IAI extends InlinedArray {
    IAI(int num) {
      super(INT_ARRAY, num);
    }
    public DataList createReadOnly(Object v, int start, int length) {
      final int[] value= (int[])v;
      return new IntArrayArray.Inlined(value, numPerEntry, start, length);
    }
  }
  static class DAA extends ArrayOf {
    DAA() {
      super(DOUBLE_ARRAY);
    }
    DAA(int num) {
      super(DOUBLE_ARRAY, num);
    }
    public DataList createReadOnly(Object v, int start, int length) {
      final double[][] value= (double[][])v;
      if(start!=0||length!=value.length)
        throw new UnsupportedOperationException("["+start+", "+(start+length)+'[');
      return super.numPerEntry==-1?
        new DoubleArrayArray.Array(value):
        new DoubleArrayArray.Array(value, super.numPerEntry);
    }
  }
  static class DAI extends InlinedArray {
    DAI(int num) {
      super(DOUBLE_ARRAY, num);
    }
    public DataList createReadOnly(Object v, int start, int length) {
      final double[] value= (double[])v;
      return new DoubleArrayArray.Inlined(value, numPerEntry, start, length);
    }
  }

  static class SAA extends ArrayOf {
    SAA() {
      super(STRING_ARRAY);
    }
    SAA(int num) {
      super(STRING_ARRAY, num);
    }
    public DataList createReadOnly(Object v, int start, int length) {
      final String[][] value= (String[][])v;
      if(start!=0||length!=value.length)
        throw new UnsupportedOperationException("["+start+", "+(start+length)+'[');
      return new StringArrayArray.Array(value);
//      return super.numPerEntry==-1?
//        new IntArrayArray.Array(value):
//        new IntArrayArray.Array(value, super.numPerEntry);
    }
  }
  static class SAI extends InlinedArray {
    SAI(int num) {
      super(STRING_ARRAY, num);
    }
    public DataList createReadOnly(Object v, int start, int length) {
      final String[] value= (String[])v;
      return new StringArrayArray.Inlined(value, numPerEntry, start, length);
    }
  }

  public void checkFormat(Object newData)
  {
    // TODO Implement
  }

  public abstract DataItem item(Object data, int i);

  void exportData(ObjectOutputStream stream, DataList list)
    throws IOException
  {
    stream.writeObject(list.data);
  }

  Object importData(ObjectInputStream stream)
    throws IOException, ClassNotFoundException
  {
    return stream.readObject();
  }
}
