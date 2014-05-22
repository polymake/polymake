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

import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Map.Entry;
import java.util.Set;

import de.jreality.scene.data.Attribute;
import de.jreality.scene.data.DataList;
import de.jreality.scene.data.DataListSet;
import de.jreality.scene.data.WritableDataList;
import de.jreality.scene.event.GeometryEvent;
import de.jreality.scene.event.GeometryEventMulticaster;
import de.jreality.scene.event.GeometryListener;

/**
 * A geometry leaf. Supports arbitrary attributes ({@link #setGeometryAttributes(String, Object)}),
 * and registering instances of {@link de.jreality.scene.event.GeometryListener}.
 * 
 * @author Unknown
 */
public abstract class Geometry extends SceneGraphNode {
	
  private static final Map<String, Object> EMPTY_GEOMETRY_ATTRIBUTE_MAP=Collections.emptyMap();
  
  public static final String CATEGORY_VERTEX = "VERTEX";
  public static final String CATEGORY_EDGE = "EDGE";
  public static final String CATEGORY_FACE = "FACE";
	
  protected Map<String, Object> geometryAttributes=Collections.emptyMap();
  private transient GeometryEventMulticaster geometryListener = new GeometryEventMulticaster();
  
  protected transient Set<String> changedGeometryAttributes=new HashSet<String>();
  protected transient Set<Attribute> changedVertexAttributes=new HashSet<Attribute>();
  protected transient Set<Attribute> changedEdgeAttributes=new HashSet<Attribute>();
  protected transient Set<Attribute> changedFaceAttributes=new HashSet<Attribute>();
  protected transient Map<String,DataListSet> geometryAttributeCategory=new HashMap<String,DataListSet>();
  
  public Geometry(String name) {
    super(name);
  }
    
  /**
   * Returns a read-only view to all currently defined geometry attributes.
   * You can copy all currently defined geometry attributes to another
   * geometry using
   * <code>target.setGeometryAttributes(source.getGeometryAttributes())</code>
   * These attributes are copied then, not shared. Thus modifying either
   * source or target afterwards will not affect the other.
   * @see setGeometryAttributes(DataListSet)
   */
  public Map<String, Object> getGeometryAttributes() {
    startReader();
    try {
      return geometryAttributes.isEmpty()?
    		  geometryAttributes : Collections.unmodifiableMap(geometryAttributes);
    } finally {
      finishReader();
    }
  }

  public Object getGeometryAttributes(String name) {
    startReader();
    try {
      return geometryAttributes.get(name);
    } finally {
      finishReader();
    }
  }
  
  public void setGeometryAttributes(Map<String, Object> attrSet) {
    checkReadOnly();
    if(attrSet.isEmpty()) return;
    startWriter();
    try {
      if(geometryAttributes==EMPTY_GEOMETRY_ATTRIBUTE_MAP)
        geometryAttributes=new HashMap<String, Object>(attrSet.size());
      for (Entry<String, Object> e : attrSet.entrySet()) {
        if(e.getValue()!=null)
          geometryAttributes.put(e.getKey(), e.getValue());
        else
          geometryAttributes.remove(e.getKey());
      }
    } finally {
      finishWriter();
    }
  }

  public void setGeometryAttributes(String attr, Object value) {
	    checkReadOnly();
	    startWriter();
	    try {
	      if(geometryAttributes==Collections.EMPTY_MAP)
	        geometryAttributes=new HashMap<String, Object>();
	      if(value!=null)
	        geometryAttributes.put(attr, value);
	      else
	        geometryAttributes.remove(attr);
	      fireGeometryAttributesChanged(Collections.singleton(attr));
	    } finally {
	      finishWriter();
	    }
  }
  
  final void setAttrImpl(DataListSet target, DataListSet data, boolean replace) {
    if(replace) target.reset(data.getListLength());
    for(Attribute a : data.storedAttributes() ) {
      setAttrImpl(target, a, data.getList(a), false);
    }
  }
  
  final void setAttrImpl(DataListSet target, Attribute a, DataList d, boolean replace) {
    if (d == null) {
      if (replace) throw new IllegalArgumentException("datalist==null for setting list length");
      target.remove(a);
    } else {
      WritableDataList w;
      if(replace) { target.reset(d.size()); w=null; } 
      else w=target.getWritableList(a);
      // problem with indices; easiest way around is to remove old indices before setting new
      // we need a way to check if the complete 2D array of the new data 
      // is the same dimension as the old; if not, we get exception when a copy
      // lacking that, we assume they're different. note that the same fix is needed
      // in de.jreality.geometry.GeometryAttributeListSet.setAttrImpl(DataListSet target, Attribute a, DataLi
      if(w==null || a == Attribute.INDICES) w=target.addWritable(a, d.getStorageModel());
      d.copyTo(w);
    }
  }

  public void addGeometryListener(GeometryListener listener) {
    startReader();
    geometryListener.add( listener);
    finishReader();
  }

  public void removeGeometryListener(GeometryListener listener) {
    startReader();
    geometryListener.remove( listener);
    finishReader();
  }

  /**
   * collect changed attributes
   */
  protected void fireGeometryDataChanged(String category, Set<Attribute> attributeKeys) {
	if (attributeKeys == null) return;
    if (category == CATEGORY_VERTEX) changedVertexAttributes.addAll(attributeKeys);
    if (category == CATEGORY_EDGE) changedEdgeAttributes.addAll(attributeKeys);
    if (category == CATEGORY_FACE) changedFaceAttributes.addAll(attributeKeys);
  }
  
  protected void fireGeometryAttributesChanged(Set<String> attributeKeys) {
	  if (attributeKeys != null) changedGeometryAttributes.addAll(attributeKeys);
  }

  /**
   * @depecated use fireGeometryData/AttributesChanged instead!
   */
  protected void fireGeometryChanged(Set<Attribute> vertexAttributeKeys,
    Set<Attribute> edgeAttributeKeys, Set<Attribute> faceAttributeKeys, Set<String> geomAttributeKeys) {
    if (vertexAttributeKeys != null) changedVertexAttributes.addAll(vertexAttributeKeys);
    if (edgeAttributeKeys != null) changedEdgeAttributes.addAll(edgeAttributeKeys);
    if (faceAttributeKeys != null) changedFaceAttributes.addAll(faceAttributeKeys);
    if (geomAttributeKeys != null) changedGeometryAttributes.addAll(geomAttributeKeys);
  }

  protected void writingFinished() {
    fireGeometryChangedImpl(changedVertexAttributes, changedEdgeAttributes, changedFaceAttributes, changedGeometryAttributes);
    changedVertexAttributes.clear();
    changedEdgeAttributes.clear();
    changedFaceAttributes.clear();
    changedGeometryAttributes.clear();
  }
  
  /**
   * The number of entries defines the length of all data lists associated
   * with the target geometry attributes.
   * @param geometryAttributes target data list set
   */
  int getNumEntries( DataListSet geometryAttributes )
  {
    startReader();
    try {
      return geometryAttributes.getListLength();
    } finally {
      finishReader();
    }
  }

  /**
   * Sets the number of entries, implies removal of all previously defined
   * such geometries attributes
   * @param geometryAttributes target data list set	
   * @param numEntries the number of vertices to set >=0
   */
  protected void setNumEntries( DataListSet geometryAttributes, int numEntries)
  {
    checkReadOnly();
    startWriter();
    try {
      geometryAttributes.reset(numEntries);
    } finally {
      finishWriter();
    }
  }

  /**
   * Returns a read-only view to all currently defined target geometry attributes.
   * @param geometryAttributes target data list set	
   * @see setVertexAttributes(DataListSet)
   * @see getGeometryAttributes()
   */
  protected DataListSet getAttributes( DataListSet geometryAttributes )
  {
    startReader();
    try {
      return geometryAttributes.readOnly();
    } finally {
      finishReader();
    }
  }

  protected DataList getAttributes( DataListSet geometryAttributes, Attribute attr) {
    startReader();
    try {
      return geometryAttributes.getList(attr);
    } finally {
      finishReader();
    }
  }

  protected void setAttributes(String category, DataListSet geometryAttributes, DataListSet dls) {
    checkReadOnly();
    startWriter();
    setAttrImpl(geometryAttributes, dls, false);
    fireGeometryDataChanged(category, dls.storedAttributes());
    finishWriter();
  }

  protected void setAttributes(String category, DataListSet geometryAttributes, Attribute attr, DataList dl) {
    checkReadOnly();
    startWriter();
    setAttrImpl( geometryAttributes, attr, dl, false);
    fireGeometryDataChanged(category, Collections.singleton(attr));
    finishWriter();
  }

  protected void setCountAndAttributes(String category, DataListSet geometryAttributes, Attribute attr, DataList dl) {
    checkReadOnly();
    startWriter();
    setAttrImpl( geometryAttributes, attr, dl, true);
    fireGeometryDataChanged(category, Collections.singleton(attr));
    finishWriter();
  }

  protected void setCountAndAttributes(String category, DataListSet geometryAttributes, DataListSet dls) {
    checkReadOnly();
    startWriter();
    setAttrImpl( geometryAttributes, dls, true);
    fireGeometryDataChanged(category, dls.storedAttributes());
    finishWriter();
  }

  public Set<String>getGeometryAttributeCathegories() {
	  return geometryAttributeCategory.keySet();
  }
  /**
   * The number of entries defines the length of all data lists associated
   * with the target geometry attributes.
   * @param attributeCategory key for target data list set
   */
  public int getNumEntries( String attributeCategory ) {
	  return getNumEntries( geometryAttributeCategory.get( attributeCategory));
  }

  /**
   * Sets the number of entries, implies removal of all previously defined
   * such geometries attributes
   * @param attributeCategory key for target data list set
   * @param numEntries the number of vertices to set >=0
   */
  public  void setNumEntries( String attributeCategory, int numEntries) {
	  setNumEntries( geometryAttributeCategory.get( attributeCategory), numEntries );
  }

  /**
   * Returns a read-only view to all currently defined target geometry attributes.
   * @param attributeCategory key for target data list set
   * @see setVertexAttributes(DataListSet)
   * @see getGeometryAttributes()
   */
  public DataListSet getAttributes( String attributeCategory ) {
	  return getAttributes(geometryAttributeCategory.get( attributeCategory));
  }

  public DataList getAttributes( String attributeCategory, Attribute attr) {
	  return getAttributes(geometryAttributeCategory.get( attributeCategory), attr);
  }

  public void setAttributes( String attributeCategory, DataListSet dls) {
	  setAttributes(attributeCategory, geometryAttributeCategory.get( attributeCategory),dls);
  }

  public void setAttributes( String attributeCategory, Attribute attr, DataList dl) {
	  setAttributes(attributeCategory, geometryAttributeCategory.get( attributeCategory), attr, dl );
  }

  public void setCountAndAttributes( String attributeCategory, Attribute attr, DataList dl) {
	  setCountAndAttributes(attributeCategory, geometryAttributeCategory.get( attributeCategory),attr,dl);
  }

  public void setCountAndAttributes( String attributeCategory, DataListSet dls) {
	  setCountAndAttributes(attributeCategory, geometryAttributeCategory.get( attributeCategory),dls);
  }

  /**
   * Tell the outside world that this geometry has changed.
   */
  protected void fireGeometryChangedImpl(Set<Attribute> vertexAttributeKeys,
    Set<Attribute> edgeAttributeKeys, Set<Attribute> faceAttributeKeys, Set<String> geomAttributeKeys) {
    final GeometryListener l=geometryListener;
    if(l != null) l.geometryChanged(new GeometryEvent(this, vertexAttributeKeys,
      edgeAttributeKeys, faceAttributeKeys, geomAttributeKeys));
  }

  public void accept(SceneGraphVisitor v) {
    startReader();
    try {
      v.visit(this);
    } finally {
      finishReader();
    }
  }
  
  static void superAccept(Geometry g, SceneGraphVisitor v) {
    g.superAccept(v);
  }

  private void superAccept(SceneGraphVisitor v) {
    super.accept(v);
  }
}
