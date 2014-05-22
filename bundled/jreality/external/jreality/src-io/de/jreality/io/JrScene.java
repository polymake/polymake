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


package de.jreality.io;

import java.util.HashMap;
import java.util.Map;

import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SceneGraphPath;

public class JrScene {
  
  private SceneGraphComponent sceneRoot;
  private Map<String, SceneGraphPath> scenePaths=new HashMap<String, SceneGraphPath>();
  private Map<String, Object> sceneAttributes=new HashMap<String, Object>();
  
  public JrScene() {
  }
  
  public JrScene(SceneGraphComponent root) {
    setSceneRoot(root);
  }
  
  public void addPath(String name, SceneGraphPath path) {
    if (!(path.getFirstElement() == sceneRoot) || !path.isValid())
      throw new IllegalArgumentException("invalid path!");
    scenePaths.put(name, path);
  }
  
  public void addAttribute(String name, Object attribute) {
    sceneAttributes.put(name, attribute);
  }

  public SceneGraphComponent getSceneRoot() {
    return sceneRoot;
  }
  
  public void setSceneRoot(SceneGraphComponent root) {
    this.sceneRoot=root;
  }

  
  /**
   * currently used:</br>
   * <ul>
   *  <li>"cameraPath"</li>
   *  <li>"emptyPickPath"</li>
   *  <li>"avatarPath"</li>
   *  <li>"microphonePath" (not yet...)</li>
   *  <li>"contentPath" (not yet...)</li>
   * </ul>
   * @param name the name of the SceneGraphPath
   * @return the corresponding SceneGraphPath
   */
  public SceneGraphPath getPath(String name) {
    return (SceneGraphPath) scenePaths.get(name);
  }

  public Object getAttribute(String name) {
    return sceneAttributes.get(name);
  }
  
  public Map<String, SceneGraphPath> getScenePaths() {
    return scenePaths;
  }
  
  public Map<String, Object> getSceneAttributes() {
    return sceneAttributes;
  }
}
