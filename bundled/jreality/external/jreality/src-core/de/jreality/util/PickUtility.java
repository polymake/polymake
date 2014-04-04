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


package de.jreality.util;

import de.jreality.scene.Appearance;
import de.jreality.scene.Geometry;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.Scene;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SceneGraphVisitor;
import de.jreality.scene.pick.AABBTree;
import de.jreality.shader.CommonAttributes;

public class PickUtility {

  public static String AABB_TREE = "AABBTree";
	
  private static final int DEFAULT_TRIANGLES_PER_BOX = 10;

  private PickUtility() {}
  
  /*
   * Do thiese methods perhaps belong in the de.jreality.pick package
   */
  public static void assignFaceAABBTree(IndexedFaceSet ifs) {
    assignFaceAABBTree(ifs, DEFAULT_TRIANGLES_PER_BOX);
  }
  
  public static void assignFaceAABBTree(IndexedFaceSet ifs, int maxTrianglesPerBox) {
	if (ifs.getNumFaces() == 0) return;
    ifs.setGeometryAttributes(AABB_TREE, AABBTree.construct(ifs, maxTrianglesPerBox));
  }
  public static void assignFaceAABBTrees(final SceneGraphComponent comp) {
    assignFaceAABBTrees(comp, DEFAULT_TRIANGLES_PER_BOX);
  }
  
  public static void assignFaceAABBTrees(final SceneGraphComponent comp, final int maxTrianglesPerBox) {
    comp.accept(new SceneGraphVisitor() {
      public void visit(SceneGraphComponent c) {
        if (c.getGeometry() != null) {
          if (c.getGeometry() instanceof IndexedFaceSet)
            assignFaceAABBTree((IndexedFaceSet) c.getGeometry(), maxTrianglesPerBox);
        }
        c.childrenAccept(this);
      }
    });
  }

  /**
   * Setting this will abort all picking for this scene graph. 
   * If this isn't desired, use {@link #setPickable(SceneGraphComponent, boolean, boolean, boolean)}.
   * @param cmp
   * @param pickable
   * @deprecated	Use {@link SceneGraphComponent#setPickable(boolean)}.
   */
  public static void setPickable(SceneGraphComponent cmp, final boolean pickable) {
	  if (cmp.getAppearance() ==null) cmp.setAppearance(new Appearance());
      cmp.getAppearance().setAttribute(CommonAttributes.PICKABLE, pickable);
  }
  
  public static void setPickable(SceneGraphComponent cmp, final boolean pickPoints, boolean pickEdges, boolean pickFaces) {
	if (cmp.getAppearance() ==null) cmp.setAppearance(new Appearance());
	  
	cmp.getAppearance().setAttribute(CommonAttributes.POINT_SHADER+"."+CommonAttributes.PICKABLE, pickPoints);
	cmp.getAppearance().setAttribute(CommonAttributes.LINE_SHADER+"."+CommonAttributes.PICKABLE, pickEdges);
	cmp.getAppearance().setAttribute(CommonAttributes.POLYGON_SHADER+"."+CommonAttributes.PICKABLE, pickFaces);

  }
  public static void setPickable(Geometry g, boolean pickable) {
    g.setGeometryAttributes(CommonAttributes.PICKABLE, pickable);
  }

  /**
   * recursively clears all pickable appearance attributes
   */
  public static void clearPickableAttributes(final SceneGraphComponent cmp) {
	  Scene.executeWriter(cmp, new Runnable() {
		 public void run() {
			cmp.accept(new SceneGraphVisitor() {
				@Override
				public void visit(Appearance a) {
					a.setAttribute("pointShader."+CommonAttributes.PICKABLE, Appearance.INHERITED);
					a.setAttribute("lineShader."+CommonAttributes.PICKABLE, Appearance.INHERITED);
					a.setAttribute("polygonShader."+CommonAttributes.PICKABLE, Appearance.INHERITED);
					a.setAttribute(CommonAttributes.PICKABLE, Appearance.INHERITED);
				}
				@Override
				public void visit(SceneGraphComponent c) {
					c.childrenWriteAccept(this, false, true, false, false, false, false);
				}
			});
		} 
	  });
  }
  

}
