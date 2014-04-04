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

import java.util.AbstractList;
import java.util.Comparator;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map.Entry;
import java.util.TreeMap;

/**
 * Base class for data items of this package supporting different views
 * to the same data using the same reference but still maintaining the
 * readonly constraint of the source.
 * @author Holger
 */
public abstract class DataItem extends AbstractList {
  final static boolean DEBUG_USAGE = false;

  final Object data;
  final int offset;

  DataItem() { data=null; offset=0; }//for deserialization only

  public DataItem(Object source, int srcOffset) {
    data=source;
    offset=srcOffset;
    if(DEBUG_USAGE) notifyNewInstance();//debugging
  }

  public abstract StorageModel getStorageModel();

  public IntArray toIntArray() {
    return getStorageModel().toIntArray(this);
  }
  public IntArrayArray toIntArrayArray() {
    return getStorageModel().toIntArrayArray(this);
  }
  public DoubleArray toDoubleArray() {
    return getStorageModel().toDoubleArray(this);
  }
  /**
   * Copy the data into the specified array or into a new one
   * if the target parameter is <code>null</code>. Returns
   * the filled array.
   */
  public double[] toDoubleArray(double[] target) {
    return (double[])copyTo(StorageModel.DOUBLE_ARRAY, target);
  }
  /**
   * Copy the data into the specified array or into a new one
   * if the target parameter is <code>null</code>. Returns
   * the filled array.
   */
  public double[][] toDoubleArrayArray(double[][] target) {
    return (double[][])copyTo(StorageModel.DOUBLE_ARRAY.array(), target);
  }
  /**
   * Copy the data into the specified array or into a new one
   * if the target parameter is <code>null</code>. Returns
   * the filled array.
   */
  public int[] toIntArray(int[] target) {
    return (int[])copyTo(StorageModel.INT_ARRAY, target);
  }
  /**
   * Copy the data into the specified array or into a new one
   * if the target parameter is <code>null</code>. Returns
   * the filled array.
   */
  public int[][] toIntArrayArray(int[][] target) {
    return (int[][])copyTo(StorageModel.INT_ARRAY_ARRAY, target);
  }
  
  
  public StringArray toStringArray() {
	    return getStorageModel().toStringArray(this);
	  }
	  public StringArrayArray toStringArrayArray() {
	    return getStorageModel().toStringArrayArray(this);
	  }
	  public String[] toStringArray(String[] target) {
		    return (String[])copyTo(StorageModel.STRING_ARRAY, target);
		  }
		  /**
		   * Copy the data into the specified array or into a new one
		   * if the target parameter is <code>null</code>. Returns
		   * the filled array.
		   */
		  public String[][] toStringArrayArray(String[][] target) {
		    return (String[][])copyTo(StorageModel.STRING_ARRAY_ARRAY, target);
		  }
		  
  
  /**
   * Copy the data into the specified array or into a new one
   * if the target parameter is <code>null</code>. Returns
   * the filled array. The passed in StorageModel specifies the
   * data format of the target. If the format is a multidim. array
   * and the target contains <code>null</code> references, the
   * storage model <em>must</em> specify an entry size as this
   * method will create the required arrays. This method will
   * apply flattening conversions if the target storage model has
   * less dimensions than the underlying model of this list.
   * The <code>to<i>XXX</i>Array</code>() methods are shorthands
   * for commonly used data formats. Note that unlike the
   * {@link List#toArray} method this method will not create a new
   * array if the target is too small.
   */
  public Object copyTo(StorageModel which, Object target) {
    return getStorageModel().copy((DataList)this, which, target);
  }

  public Object copyTo(WritableDataList target) {
    return getStorageModel().copy((DataList)this,
      target.getStorageModel(), target.getData());
  }
  // debugging code below

  static {
    if(DEBUG_USAGE) Runtime.getRuntime().addShutdownHook(
      new Thread("DataItem Usage Debugger") {
        public void run() {
          printUsage();
        }
      });
  }
  static final HashMap USAGE= new HashMap();
  private final void notifyNewInstance() {
    Class cl= getClass();
    int[] count= (int[])USAGE.get(cl);
    if (count == null)
      USAGE.put(cl, count= new int[1]);
    count[0]++;
  }
  /**
   * For debugging only: print the usage count of all data items.
   * <em>Do not use in production code, this method might disappear
   * without further notice</em>.
   */
  public static void printUsage()
  {
    Comparator c=new Comparator() {
      public int compare(Object o1, Object o2) {
        String s1=((Class)o1).getName(), s2=((Class)o2).getName();
        return s1.compareTo(s2);
      }
    };
    TreeMap ts=new TreeMap(c); ts.putAll(USAGE);
    for(Iterator it=ts.entrySet().iterator(); it.hasNext();)
    {
      Entry e=(Entry)it.next();
      Class cl=(Class)e.getKey();
      int v=((int[])e.getValue())[0];
      String name=cl.getName();
      if(name.startsWith("de.jreality.scene.data."))
        name=name.substring("de.jreality.scene.data.".length());
      name=name.replace('$', '.');
      System.out.println(name+": "+v);
    }
  }

}
