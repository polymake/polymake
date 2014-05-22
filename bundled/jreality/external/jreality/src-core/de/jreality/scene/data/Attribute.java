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

import java.io.ObjectStreamException;
import java.io.Serializable;
import java.util.HashMap;

/**
 * @author Holger
 */
public class Attribute implements Serializable {
  
  private static final HashMap<String, Attribute> KNOWN_ATTRIBUTES = new HashMap<String, Attribute>();

  public static final Attribute COORDINATES=attributeForName("coordinates");
  public static final Attribute NORMALS=attributeForName("normals");
  public static final Attribute COLORS=attributeForName("colors");
  public static final Attribute INDICES=attributeForName("indices");
  public static final Attribute TEXTURE_COORDINATES=attributeForName("texture coordinates");
  public static final Attribute TEXTURE_COORDINATES1=attributeForName("texture coordinates 1");
  public static final Attribute TEXTURE_COORDINATES2=attributeForName("texture coordinates 2");
  public static final Attribute POINT_SIZE=attributeForName("pointSize");
  public static final Attribute RELATIVE_RADII=attributeForName("relativeRadii");
  public static final Attribute LABELS=attributeForName("labels");
  
  /**
   * might be a threading problem
   */
  public static Attribute attributeForName(String name) {
	Attribute aa = KNOWN_ATTRIBUTES.get(name);
    if (aa != null) return aa;
    aa = new Attribute(name);
    KNOWN_ATTRIBUTES.put(name, aa);
    return aa;
  }

  private final String attrName;

  private Attribute(String name) {
    attrName=name;
  }

  public String getName() {
    return attrName;
  }

  public String toString() {
    return attrName;
  }
  
  Object readResolve() throws ObjectStreamException
  {
    return attributeForName(getName());
  }
}
