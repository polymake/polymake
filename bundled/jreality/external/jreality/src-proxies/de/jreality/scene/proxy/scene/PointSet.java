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


package de.jreality.scene.proxy.scene;

import java.util.Collections;
import java.util.Iterator;

import de.jreality.scene.data.Attribute;
import de.jreality.scene.data.ByteBufferList;
import de.jreality.scene.data.DataList;
import de.jreality.scene.data.DataListSet;
import de.jreality.scene.data.StorageModel;
import de.jreality.scene.data.WritableDataList;

public class PointSet extends de.jreality.scene.PointSet implements RemotePointSet {
    
  public void setVertexCountAndAttributes(DataListSet dls) {
    startWriter();
    try {
      PointSet.setAttrImp(vertexAttributes, dls, true);
      fireGeometryDataChanged(CATEGORY_VERTEX, dls.storedAttributes());
    } finally {
      finishWriter();
    }
  }
  
  public void setVertexAttributes(DataListSet dls) {
    startWriter();
    try {
      PointSet.setAttrImp(vertexAttributes, dls, dls.getListLength() != vertexAttributes.getListLength());
      fireGeometryDataChanged(CATEGORY_VERTEX, dls.storedAttributes());
    } finally {
      finishWriter();
    }
  }
  
  public void setVertexAttributes(Attribute attr, DataList dl) {
    startWriter();
    try {
    	if (dl == null) {
    		vertexAttributes.remove(attr);
    	} else {
	      int length = (dl instanceof ByteBufferList) ? ((ByteBufferList)dl).getCoveredLength() : dl.size();
	      PointSet.setAttrImp(vertexAttributes, attr, dl, length != vertexAttributes.getListLength());
    	}
        fireGeometryDataChanged(CATEGORY_VERTEX, Collections.singleton(attr));
    } finally {
      finishWriter();
    }
  }

  public void setVertexCountAndAttributes(Attribute attr, DataList dl) {
    startWriter();
    try {
      PointSet.setAttrImp(vertexAttributes, attr, dl, true);
      fireGeometryDataChanged(CATEGORY_VERTEX, Collections.singleton(attr));
    } finally {
      finishWriter();
    }
  }

  final static void setAttrImp(DataListSet target, DataListSet data, boolean replace) {
    if(replace) target.reset(data.getListLength());
    for(Iterator i=data.storedAttributes().iterator(); i.hasNext(); ) {
      Attribute a=(Attribute)i.next();
      PointSet.setAttrImp(target, a, data.getList(a), false);
    }
  }
  
  final static void setAttrImp(DataListSet target, Attribute a, DataList d, boolean replace) {
    StorageModel sm = d.getStorageModel();
    ByteBufferList bbl = null;
    int length = d.size();
    boolean isBBList = d instanceof ByteBufferList;
    if (isBBList) {
        bbl = (ByteBufferList) d;
        sm = bbl.getCoveredModel();
        length = bbl.getCoveredLength();
    }
    WritableDataList w;
    if(replace) { target.reset(length); w=null; } 
    else w=target.getWritableList(a);
    if(w==null) {
        w=isBBList?target.addWritable(a, sm, bbl.createFittingDataObject()) : target.addWritable(a, sm);
    }
    d.copyTo(w);
  }

}
