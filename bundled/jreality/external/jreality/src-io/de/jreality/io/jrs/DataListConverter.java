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


package de.jreality.io.jrs;

import java.util.regex.Matcher;
import java.util.regex.Pattern;

import com.thoughtworks.xstream.converters.MarshallingContext;
import com.thoughtworks.xstream.converters.UnmarshallingContext;
import com.thoughtworks.xstream.io.HierarchicalStreamReader;
import com.thoughtworks.xstream.io.HierarchicalStreamWriter;
import com.thoughtworks.xstream.mapper.Mapper;

import de.jreality.scene.data.DataList;
import de.jreality.scene.data.DoubleArray;
import de.jreality.scene.data.DoubleArrayArray;
import de.jreality.scene.data.IntArray;
import de.jreality.scene.data.IntArrayArray;
import de.jreality.scene.data.StringArray;
import de.jreality.scene.data.StringArrayArray;

class DataListConverter extends AbstractConverter {
  
  public DataListConverter(Mapper mapper, double version) {
		super(mapper, version);
	}

Pattern arrayPattern = Pattern.compile("[^\\[\\]]*\\[\\]");
  Pattern arrayArrayPattern = Pattern.compile("[^\\[\\]]*\\[\\]\\[\\]");
  Pattern arrayArrayInlinedPattern = Pattern.compile("[^\\[\\]]*\\[\\]\\[(([0-9])+)\\]");
  
  public boolean canConvert(Class type) {
    return DataList.class.isAssignableFrom(type);
  }

  public void marshal(Object source, HierarchicalStreamWriter writer, MarshallingContext context) {
    DataList dl = (DataList) source;
    String sm = dl.getStorageModel().toString();
    writer.addAttribute("data", sm);
    Object dst=null;
    if (sm.startsWith("double")) {
      if (isArray(sm) || isInlined(sm)) {
        double[] data = dl.toDoubleArray(null);
        dst = data;
      } else if (isArrayArray(sm)) {
        double[][] data = dl.toDoubleArrayArray(null);
        dst = data;
      }
    } else if (sm.startsWith("int")) {
      if (isArray(sm) || isInlined(sm)) {
        int[] data = dl.toIntArray(null);
        dst = data;
      } else if (isArrayArray(sm)) {
        int[][] data = dl.toIntArrayArray(null);
        dst = data;
      }
    } else if (sm.startsWith("String")) {
      if (isArray(sm) || isInlined(sm)) {
        String[] data = dl.toStringArray(null);
        dst = data;
      } else if (isArrayArray(sm)) {
        String[][] data = dl.toStringArrayArray(null);
        dst = data;
      }
    } else {
      throw new UnsupportedOperationException("cannot write: "+sm);
    }
    if (version < 0.2) writer.startNode(mapper.serializedClass(DataList.class));
    context.convertAnother(dst);
    if (version < 0.2) writer.endNode();
  }

  public Object unmarshal(HierarchicalStreamReader reader,
      UnmarshallingContext context) {
    String sm = reader.getAttribute("data");
    Object ret = null;
    if (version < 0.2) reader.moveDown();
    if (sm.startsWith("double")) {
      if (isArray(sm)) {
        double[] data = (double[]) context.convertAnother(null, double[].class);
        ret = new DoubleArray(data);
      } else if (isInlined(sm)) {
        double[] data = (double[]) context.convertAnother(null, double[].class);
        ret =  new DoubleArrayArray.Inlined(data, slotLength(sm));
      } else if (isArrayArray(sm)) {
        double[][] data = (double[][]) context.convertAnother(null, double[][].class);
        ret = new DoubleArrayArray.Array(data);
      }
    } else if (sm.startsWith("int")) {
      if (isArray(sm)) {
        int[] data = (int[]) context.convertAnother(null, int[].class);
        ret = new IntArray(data);
      } else if (isInlined(sm)) {
        int[] data = (int[]) context.convertAnother(null, int[].class);
        ret = new IntArrayArray.Inlined(data, slotLength(sm));
      } else if (isArrayArray(sm)) {
        int[][] data = (int[][]) context.convertAnother(null, int[][].class);
        ret = new IntArrayArray.Array(data);
      }
    } else if (sm.startsWith("String")) {
      if (isArray(sm)) {
        String[] data = (String[]) context.convertAnother(null, String[].class);
        ret = new StringArray(data);
      } else if (isInlined(sm)) {
        String[] data = (String[]) context.convertAnother(null, String[].class);
        ret = new StringArrayArray.Inlined(data, slotLength(sm));
      } else if (isArrayArray(sm)) {
        String[][] data = (String[][]) context.convertAnother(null, String[][].class);
        ret = new StringArrayArray.Array(data);
      }
    } else {
      throw new UnsupportedOperationException("cannot read: "+sm);
    }
    if (version < 0.2) reader.moveUp();
    return ret;
  }

  private int slotLength(String sm) {
    Matcher m = arrayArrayInlinedPattern.matcher(sm);
    if (!m.find()) throw new IllegalArgumentException("no length!");
//    for (int i = 0; i<m.groupCount(); ++i)	{
//    		System.err.println("Group "+i+" = "+m.group(i));
//    }
    String foo = m.group(1);
    return Integer.parseInt(foo);
  }

  private boolean isArrayArray(String sm) {
    return arrayArrayPattern.matcher(sm).matches();
  }

  private boolean isInlined(String sm) {
    return arrayArrayInlinedPattern.matcher(sm).matches();
  }

  private boolean isArray(String sm) {
    return arrayPattern.matcher(sm).matches();
  }
  
}
