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

import java.awt.Dimension;


/**
 * The Viewer interface represents a renderable 3D scene.
 * It consists of specifying a {@link SceneGraphComponent}, the scene root, 
 * and a path in the scene graph to a {@link Camera}. 
 * Each implementation of this interface 
 * represents a different "backend".  See separate packages for details. 
 * @author Charles Gunn, Steffen Weissmann
 */
public interface Viewer {
  
  /**
   * Get the scene root.
   * @return the scene root
   */
	public SceneGraphComponent getSceneRoot();
	
  /**
   * Set the scene root.
   * @param root the scene root
   */
  public void setSceneRoot(SceneGraphComponent root);
	
  /**
   * Get the camera path.
   * @return the camera path
   */
  public SceneGraphPath getCameraPath();
  
  /**
   * Set the camera path. Some backends assume that this
   * is a valid (existing) path starting at the scene root.
   * This implies that one first needs to set the scene root.
   * 
   * A camera path must have a Camera as the last element.
   * 
   * @param cameraPath the camera path.
   */
	public void setCameraPath(SceneGraphPath cameraPath);
  
  /**
   * This method triggers rendering of the viewer. The method returns
   * as soon as the rendering is finished.
   */
	public void render();
  
  /**
   * Has this viewer a viewing component?
   * @return true if the viewer has a viewing component,
   * false otherwise.
   */
	public boolean hasViewingComponent();
  
  /**
   * Gives the viewing component. This is a java.awt.Component for
   * an AWT viewer, or an org.eclipse.swt.Widget for an SWT viewer.
   * Maybe there will be other types of viewing components that we
   * do not know yet.
   * 
   * @return for now: a java.awt.Component or a org.eclipse.swt.Widget
   */
	public Object getViewingComponent();
  
  /**
   * Gives the dimension of the viewing component in pixel.
   * 
   * @return the Dimension of the viewing component when
   * hasViewingComponent() returns true - null otherwise.
   */
  public Dimension getViewingComponentSize();
  
  /**
   * Some viewers (at least the jogl viewer) support rendering
   * of non-euclidean geometries. The definition of the metric is
   * in de.jreality.math.Pn.
   * 
   * @return the metric of the viewer.
   * @see de.jreality.math.Pn
   * @deprecated	Use {@link Appearance#getAttribute("metric")}.
   */
//	public int getMetric();
  
  /**
   * Some viewers (at least the jogl viewer) support rendering
   * of non-euclidean geometries. The definition of the metric is
   * in de.jreality.math.Pn.
   * 
   * @param the metric of the viewer as defined in de.jreality.math.Pn
   * @see de.jreality.math.Pn
   * @deprecated  Use {@link Appearance#setAttribute("metric", Integer)}
   */
//	public void setMetric(int sig);
  
  /**
   * Some viewers (at least the jogl viewer) support an auxilary
   * root - for things to display that are no part of the scene itself.
   * I. e. bounding boxes should go here.
   *
   * @param auxRoot the auxiliary root
   */
  public void setAuxiliaryRoot(SceneGraphComponent auxRoot);
  
  /**
   * Some viewers (at least the jogl viewer) support an auxilary
   * root - for things to display that are no part of the scene itself.
   * I. e. bounding boxes should go here.
   *
   * @return the auxiliary root
   */
	public SceneGraphComponent getAuxiliaryRoot();

  /**
   * Tells wether this viewer supports asyncronous rendering.
   * 
   * @return true if the viewer supports renderAsync() false otherwise
   */
  public boolean canRenderAsync();
  
  /**
   * Some viewers support asyncronous rendering. This means: when
   * this method is called, it returns immediately and the viewer
   * renders again as soon as possible: either right now or
   * when the current rendering has finished. Multiple calls
   * of this method while the viewer is rendering trigger one
   * single rendering after the current one (optional operation).
   */
  public void renderAsync();
}
