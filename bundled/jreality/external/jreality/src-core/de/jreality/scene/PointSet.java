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


package de.jreality.scene;

import de.jreality.scene.data.Attribute;
import de.jreality.scene.data.DataList;
import de.jreality.scene.data.DataListSet;

/**
 * A set of points in 3 space. 
 * <p>
 * Vertices can be specified with either 3- or 4-D coordinates. The point set is represented as set of
 * attribute/value pairs. The values are typically arrays of data, with one vector or scalar per point.
 * Every point set must contain a value for the <code>{@link Attribute#COORDINATES}</code>.
 * Other built-in attributes include:
 * <ul>
 * <li>{@link Attribute#COLORS}
 * <li>{@link Attribute#NORMALS}
 * <li>{@link Attribute#TEXTURE_COORDINATES}
 * <li>{@link Attribute#LABELS}
 * </ul>
 * Accessing instances of this class directly requires use of the class {@link DataList}.  Users who prefer to
 * avoid this are recommended to use {@link de.jreality.geometry.PointSetFactory}
 * to construct instances of this class.
 * <p>
 * @author <a href="mailto:hoffmann@math.tu-berlin.de">Tim Hoffmann</a>, gunn
 *
 */
public class PointSet extends Geometry //implements GeometryListener
{
protected DataListSet vertexAttributes;
  
  private static int UNNAMED_ID;
  
  public PointSet()
  {
    this(0);
  }

  public PointSet(int numPoints)
  {
	this("point-set "+(UNNAMED_ID), numPoints);
 }

  public PointSet(String name)
  {
	this(name, 0);
  }

  public PointSet(String name, int numPoints)
  {
	super(name);
	vertexAttributes= new DataListSet(numPoints);
	geometryAttributeCategory.put( Geometry.CATEGORY_VERTEX, vertexAttributes );
 }

  /**
   * The number of vertices defines the length of all data lists associated
   * with vertex attributes.
   */
  public int getNumPoints() {
	return getNumEntries( vertexAttributes );
  }

  /**
   * Sets the number of vertices, implies removal of all previously defined
   * vertex attributes.
   * @param numVertices the number of vertices to set >=0
   */
  public void setNumPoints(int numVertices) {
	  setNumEntries( vertexAttributes, numVertices );
  }

  /**
   * Returns a read-only view to all currently defined vertex attributes.
   * You can copy all currently defined vertex attributes to another
   * PointSet using
   * <code>target.setVertexAttributes(source.getVertexAttributes())</code>
   * These attributes are copied then, not shared. Thus modifying either
   * source or target afterwards will not affect the other.
   * @see setVertexAttributes(DataListSet)
   * @see getGeometryAttributes()
   */
  public DataListSet getVertexAttributes() {
	  return getAttributes(vertexAttributes);
  }

  public DataList getVertexAttributes(Attribute attr) {
    return getAttributes(vertexAttributes,attr);
  }

  public void setVertexAttributes(DataListSet dls) {
	  setAttributes(CATEGORY_VERTEX, vertexAttributes,dls);
  }

  public void setVertexAttributes(Attribute attr, DataList dl) {
	  setAttributes(CATEGORY_VERTEX, vertexAttributes, attr, dl );
  }

  public void setVertexCountAndAttributes(Attribute attr, DataList dl) {
    setCountAndAttributes(CATEGORY_VERTEX, vertexAttributes, attr, dl );
  }

  public void setVertexCountAndAttributes(DataListSet dls) {
    setCountAndAttributes(CATEGORY_VERTEX, vertexAttributes,dls);
  }

  
  public void accept(SceneGraphVisitor v)
  {
    startReader();
    try {
      v.visit(this);
    } finally {
      finishReader();
    }
  }
  static void superAccept(PointSet ps, SceneGraphVisitor v)
  {
    ps.superAccept(v);
  }
  private void superAccept(SceneGraphVisitor v)
  {
    super.accept(v);
  }

}
