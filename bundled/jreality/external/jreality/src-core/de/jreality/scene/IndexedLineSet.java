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
 * A geometric object consisting of a set of edges: lists of vertices joined by line
 * segments. 
 * <p>
 * Accessing instances of this class directly requires use of the class {@link DataList}.  Users who prefer to
 * avoid this are recommended to use {@link de.jreality.geometry.IndexedLineSetFactory}
 * to construct instances of this class.
 * <p>
 * @author <a href="mailto:hoffmann@math.tu-berlin.de">Tim Hoffmann</a>, gunn
 */
public class IndexedLineSet extends PointSet
{
  private static int UNNAMED_ID;
  protected DataListSet edgeAttributes;

  public IndexedLineSet()
  {
    this("line-set ["+(UNNAMED_ID++)+"]");
  }
  
  public IndexedLineSet(String name)
  {
    this(name, 0, 0);
  }
  
  public IndexedLineSet(int numPoints)
  {
    this("line-set "+(UNNAMED_ID++), numPoints);
  }
  
  public IndexedLineSet(String name, int numPoints)
  {
    this(name, numPoints, 0);
  }

  public IndexedLineSet(int numPoints, int numEdges)
  {
    this("line-set "+(UNNAMED_ID++), numPoints, numEdges);
  }

  public IndexedLineSet(String name, int numPoints, int numEdges)
  {
    super(name, numPoints);
    edgeAttributes=new DataListSet(numEdges);
    geometryAttributeCategory.put( Geometry.CATEGORY_EDGE, edgeAttributes );
  }

  /**
   * The number of edges defines the length of all data lists associated
   * with edge attributes.
   */
  public int getNumEdges() {
    return getNumEntries(edgeAttributes);
  }

  /**
   * Sets the number of edges, implies removal of all previously defined
   * edge attributes.
   * @param numEdges the number of edges to set >=0
   */
  public void setNumEdges(int numEdges)
  {
	  if( numEdges == getNumEdges() )
		  return;
    setNumEntries( edgeAttributes, numEdges );
  }

  /**
   * Returns a read-only view to all currently defined edge attributes.
   * You can copy all currently defined edge attributes to another
   * IndexedLineSet using
   * <code>target.setEdgeAttributes(source.getEdgeAttributes())</code>
   * These attributes are copied then, not shared. Thus modifying either
   * source or target afterwards will not affect the other.
   * @see setEdgeAttributes(DataListSet)
   * @see getVertexAttributes()
   * @see getGeometryAttributes()
   */
  public DataListSet getEdgeAttributes() {
	  return getAttributes(edgeAttributes);
  }

  public DataList getEdgeAttributes(Attribute attr) {
	  return getAttributes(edgeAttributes, attr);
  }

  public void setEdgeAttributes(DataListSet dls) {
	  setAttributes(CATEGORY_EDGE, edgeAttributes,dls);
  }

  public void setEdgeAttributes(Attribute attr, DataList dl) {
	  setAttributes(CATEGORY_EDGE, edgeAttributes, attr, dl );
  }

  public void setEdgeCountAndAttributes(Attribute attr, DataList dl) {
	  setCountAndAttributes(CATEGORY_EDGE, edgeAttributes,attr,dl);
  }

  public void setEdgeCountAndAttributes(DataListSet dls) {
    setCountAndAttributes(CATEGORY_EDGE, edgeAttributes, dls);
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
  static void superAccept(IndexedLineSet g, SceneGraphVisitor v)
  {
    g.superAccept(v);
  }
  private void superAccept(SceneGraphVisitor v)
  {
    super.accept(v);
  }
}
