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
 * A geometry specified as a combinatorial set of faces. (See {@link PointSet}).
 * <p>
 * Accessing instances of this class directly requires use of the class {@link DataList}.  Users who prefer to
 * avoid this are recommended to use {@link de.jreality.geometry.IndexedFaceSetFactory}
 * and its subclasses
 * to construct instances of this class.
 * <p>
 * @author <a href="mailto:hoffmann@math.tu-berlin.de">Tim Hoffmann</a>, gunn
 *
 */
public class IndexedFaceSet extends IndexedLineSet {
  private static int UNNAMED_ID;
  protected DataListSet faceAttributes;

  public IndexedFaceSet(int numVertices, int numFaces) {
    this("face-set "+(UNNAMED_ID++), numVertices, numFaces);
  }
  
  public IndexedFaceSet(String name, int numVertices, int numFaces) {
    super(name, numVertices);
    faceAttributes= new DataListSet(numFaces);
    geometryAttributeCategory.put( Geometry.CATEGORY_FACE, faceAttributes );
  }

  public IndexedFaceSet() {
	  this("face-set "+(UNNAMED_ID++));
  }

  public IndexedFaceSet(String name) {
	  this(name,0,0);
  }

  public int getNumFaces() {
    return getNumEntries(faceAttributes);
  }

  /**
   * Sets the number of face, implies removal of all previously defined
   * face attributes.
   * @param numVertices the number of vertices to set >=0
   */
  public void setNumFaces(int numFaces) {
    setNumEntries(faceAttributes,numFaces);
  }

  /**
   * Returns a read-only view to all currently defined face attributes.
   * You can copy all currently defined face attributes to another
   * IndexedFaceSet using
   * <code>target.setFaceAttributes(source.getFaceAttributes())</code>
   * These attributes are copied then, not shared. Thus modifying either
   * source or target afterwards will not affect the other.
   * @see setFaceAttributes(DataListSet)
   * @see getEdgeAttributes()
   * @see getVertexAttributes()
   * @see getGeometryAttributes()
   */
  public DataListSet getFaceAttributes() {
    return getAttributes(faceAttributes);
  }

  public DataList getFaceAttributes(Attribute attr) {
    return getAttributes(faceAttributes, attr);
  }

  public void setFaceAttributes(DataListSet dls) {
    setAttributes(CATEGORY_FACE, faceAttributes, dls );
  }

  public void setFaceAttributes(Attribute attr, DataList dl) {
	  setAttributes(CATEGORY_FACE, faceAttributes, attr, dl);
  }

  public void setFaceCountAndAttributes(Attribute attr, DataList dl) {
    setCountAndAttributes(CATEGORY_FACE, faceAttributes,attr,dl);
  }

  public void setFaceCountAndAttributes(DataListSet dls) {
	  setCountAndAttributes(CATEGORY_FACE, faceAttributes, dls);
  }

  public void accept(SceneGraphVisitor v) {
    startReader();
    try {
      v.visit(this);
    } finally {
      finishReader();
    }
  }
  static void superAccept(IndexedFaceSet ifs, SceneGraphVisitor v) {
    ifs.superAccept(v);
  }
  private void superAccept(SceneGraphVisitor v) {
    super.accept(v);
  }

}
