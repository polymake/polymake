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


package de.jreality.shader;

import java.io.IOException;
import java.io.ObjectStreamException;
import java.io.Reader;
import java.io.Serializable;
import java.util.Collection;
import java.util.Collections;
import java.util.HashMap;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import de.jreality.util.Input;

/**
 * 
 * An abstraction of OpenGL shading language (GLSL) programs.
 * 
 * program parsing is very poor!!
 * 
 * @see de.jreality.shader.GlslProgram
 * @author Steffen Weissmann
 *
 */
public class GlslSource implements Serializable {

  private String[] vertexProgs;
  private String[] fragmentProgs;
  
  private transient final HashMap<String, UniformParameter> uniforms = new HashMap<String, UniformParameter>();
  private transient final HashMap<String, AttributeParameter> attribs = new HashMap<String, AttributeParameter>();
  private transient final Collection<UniformParameter> UNIFORMS = Collections.unmodifiableCollection(uniforms.values());
  private transient final Collection<AttributeParameter> ATTRIBUTES = Collections.unmodifiableCollection(attribs.values());
  
  private static final Pattern uniformPattern = Pattern.compile(
    "^[\\w]*uniform[\\s]+([\\w]+)[\\s]+([\\w,\t ]+)[\\s]*\\[?[\\s]*([0-9]*)[\\s]*\\]?[\\s;]+", Pattern.MULTILINE
  );
  private static final Pattern attribPattern = Pattern.compile(
		    "^[\\w]*attribute[\\s]+([\\w]+)[\\s]+([\\w,\t ]+)[\\s]*\\[?[\\s]*([0-9]*)[\\s]*\\]?[\\s;]+", Pattern.MULTILINE
		  );
  
  public GlslSource(Input vertexProgram, Input fragmentProgram) throws IOException {
      vertexProgs = vertexProgram == null ? null : new String[]{readString(vertexProgram)};
      fragmentProgs = fragmentProgram == null ? null : new String[]{readString(fragmentProgram)};
      extractParams();
  }
  
  public GlslSource(String vertexProgram, String fragmentProgram) {
    this.vertexProgs = vertexProgram == null ? null : new String[]{new String(vertexProgram)};
    this.fragmentProgs = fragmentProgram == null ? null : new String[]{new String(fragmentProgram)};
    extractParams();
  }

  public GlslSource(String[] vertexProgram, String[] fragmentProgram) {
    this.vertexProgs = vertexProgram;
    this.fragmentProgs = fragmentProgram;
    extractParams();
  }
  
  private void extractParams() {
    if (vertexProgs != null) for (int i = 0; i < vertexProgs.length; i++) {
      extractUniforms(vertexProgs[i]);
      extractAttribs(vertexProgs[i]);
    }
    if (fragmentProgs != null) for (int i = 0; i < fragmentProgs.length; i++) {
      extractUniforms(fragmentProgs[i]);
      extractAttribs(fragmentProgs[i]);
    }
  }
  
  private void extractUniforms(String prog) {
	    Matcher m = uniformPattern.matcher(prog);
	    while (m.find()) {
	      String type = m.group(1);
	      int arrayLen = -1;
	      try {
	        arrayLen = Integer.parseInt(m.group(3));
	      } catch (Exception e) {}
	      String nameStr = m.group(2);
	      String[] names = nameStr.split(", ");
	      for (int i = 0; i < names.length; i++) {
	        UniformParameter param = new UniformParameter(names[i], type, arrayLen);
	        uniforms.put(names[i], param);
	        //System.out.println("found "+param);
	      }
	    }
	  }

  private void extractAttribs(String prog) {
	    Matcher m = attribPattern.matcher(prog);
	    while (m.find()) {
	      //String type = m.group(1);
	      //int arrayLen = -1;
	      //try {
	      //  arrayLen = Integer.parseInt(m.group(3));
	      //} catch (Exception e) {}
	      String nameStr = m.group(2);
	      String[] names = nameStr.split(", ");
	      for (int i = 0; i < names.length; i++) {
	        AttributeParameter param = new AttributeParameter(names[i]);
	        attribs.put(names[i], param);
//	        System.out.println("found attrib "+param);
	      }
	    }
	  }

  public Collection<UniformParameter> getUniformParameters() {
	    return UNIFORMS;
	  }
	  
  public Collection<AttributeParameter> getAttributes() {
	    return ATTRIBUTES;
	  }
	  
  public AttributeParameter getAttribute(String name)	{
	  return (AttributeParameter) attribs.get(name);
  }
  
  public UniformParameter getUniformParameter(String name) {
    return (UniformParameter) uniforms.get(name);
  }
  
  public String[] getVertexProgram() {
    return vertexProgs;
  }
  
  public String[] getFragmentProgram() {
    return fragmentProgs;
  }
 
  private static String readString(Input input) throws IOException {
    StringBuffer sb = new StringBuffer();
    char[] buf = new char[255];
    Reader r = input.getReader();
    int read = -1;
    while ((read = r.read(buf)) != -1)
      sb.append(buf, 0, read);
    return sb.toString();
  }
  
  public class AttributeParameter {
    private final String name;
    
    private AttributeParameter(String name) {
    	this.name=name;
    }
    
    public String toString() {
        return "attribute="+name;
      }

	public String getName() {
		return name;
	}

  }
  
  public class UniformParameter {
    private final String name, type;
    private final boolean isArray;
    private final boolean isMatrix;
    private final Class primitiveType;
    private final int primitiveSize;
    private final int dataSize;
    private final int arrayLength;
    private final String stringRep;
    
    private UniformParameter(String name, String type, int arrayLen) {
      this.name=name;
      this.type=type;
      this.isArray=arrayLen > 0;
      this.isMatrix=type.startsWith("mat");
      this.arrayLength = arrayLen;
      int s=1;
      try {
        s = Integer.parseInt(type.substring(type.length()-1));
      } catch (Exception e) {}
      primitiveSize = s;
	  if (type.startsWith("i") || type.startsWith("sampler") || type.startsWith("bool"))
	        primitiveType =  int.class;
		  else 
			  primitiveType = float.class;
      int prDataSize = isMatrix ? primitiveSize*primitiveSize : primitiveSize;
      dataSize = isArray ? arrayLen * prDataSize : prDataSize;
      StringBuffer sb = new StringBuffer("glUniform");
      if (isMatrix()) sb.append("Matrix");
      sb.append(getPrimitiveSize());
      sb.append(getPrimitiveType() == float.class ? 'f' : 'i');
//      System.out.println("name="+name+" type="+type+" primitiveType="+getPrimitiveType());
      sb.append('v');
      //sb.append("ARB");
      stringRep = sb.toString();
   }
    
    public boolean isArray() {
      return isArray;
    }

    public String getName() {
      return name;
    }

    public String getType() {
      return type;
    }

    public boolean isMatrix() {
      return isMatrix;
    }
    
    public int getPrimitiveSize() {
      return primitiveSize;
    }
    
    public int getDataSize() {
      return dataSize;
    }
    
    public Class getPrimitiveType() {
      return primitiveType;
    }
    
    public int getArrayLength() {
      if (isArray) return arrayLength;
      throw new IllegalStateException("no array");
    }
    
    public String toString() {
      return "uniform: "+type+" "+name+" ["+(isArray? ("arrayLength="+arrayLength) : "")+" matrix="+isMatrix+" primitive="+primitiveType+" size="+primitiveSize+"]";
    }

	public String getStringRep() {
		// TODO Auto-generated method stub
		return stringRep;
	}
  }
  
  private void readObject(java.io.ObjectInputStream stream) throws IOException, ClassNotFoundException {
	 stream.defaultReadObject();
  }
  
  protected Object readResolve() throws ObjectStreamException {
	  return new GlslSource(vertexProgs, fragmentProgs);
  }
  
  public static void main(String[] args) throws Exception {
    GlslSource source = new GlslSource(
        Input.getInput("de/jreality/jogl/shader/resources/brick.vert"),
        Input.getInput("de/jreality/jogl/shader/resources/brick.frag")
    );
  }
}
