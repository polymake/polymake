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

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashSet;
import java.util.LinkedList;
import java.util.List;

import de.jreality.scene.event.SceneEvent;
import de.jreality.scene.event.SceneGraphComponentEvent;
import de.jreality.scene.event.SceneGraphComponentEventMulticaster;
import de.jreality.scene.event.SceneGraphComponentListener;
import de.jreality.scene.event.ToolEvent;
import de.jreality.scene.event.ToolEventMulticaster;
import de.jreality.scene.event.ToolListener;
import de.jreality.scene.tool.Tool;

/**
 * <p>This basic building block of the jReality scene graph. It's the
 * only node that can have another SceneGraphComponent instance as a
 * child (see {@link #addChild(SceneGraphComponent)}).</p>
 *
 * <p>A SceneGraphComponent can contain other instances of
 * {@link SceneGraphNode}. To be exact, it can have one each of the
 * following: {@link Appearance}, {@link Transformation},
 * {@link Geometry}, {@link Light}, {@link Camera}, and {@link AudioSource}.
 * It also has a list of {@link Tool} instances, which may be empty.</p>
 *
 * <p>To traverse the resulting scene graph, use subclasses of
 * {@link de.jreality.scene.SceneGraphVisitor}.</p>
 *
 * @author Unknown
 * 
 */
public class SceneGraphComponent extends SceneGraphNode {
  
  private Transformation transformation;
  private Appearance appearance;
  private Camera camera;
  private Light light;
  private Geometry geometry;
  private AudioSource audioSource;
  private boolean visible = true,
  	pickable = true;
  protected List<SceneGraphComponent> children = Collections.emptyList();
  protected List<Tool> tools = Collections.emptyList();

  private transient ToolEventMulticaster toolListener = new ToolEventMulticaster();
  private transient SceneGraphComponentEventMulticaster containerListener = new SceneGraphComponentEventMulticaster();
  
  private transient List<SceneEvent> cachedEvents = new LinkedList<SceneEvent>();

  private static int UNNAMED_ID;

  public SceneGraphComponent(String name) {
	  super(name);
  }

  public SceneGraphComponent() {
	  this("sgc " + (UNNAMED_ID++));
  }

  public List<SceneGraphNode> getChildNodes() {
    startReader();
    try {
      ArrayList<SceneGraphNode> list=new ArrayList<SceneGraphNode>();
      if(transformation!=null) list.add(transformation);
      if(appearance!=null) list.add(appearance);
      if(camera!=null) list.add(camera);
      if(light!=null) list.add(light);
      if(geometry!=null) list.add(geometry);
      if(audioSource!=null) list.add(audioSource);
      list.addAll(children);
      return list.isEmpty() ? Collections.EMPTY_LIST : list;
    } finally {
      finishReader();
    }
  }
  
  public void addChild(SceneGraphComponent sgc) {
    //new Exception("addChild").printStackTrace();
    class CheckLoop extends SceneGraphVisitor {
      final HashSet<SceneGraphComponent> encountered=new HashSet<SceneGraphComponent>();
      public void visit(SceneGraphComponent c)
      {
        if(c==SceneGraphComponent.this)
          throw new SceneGraphLoopException();
        if(encountered.add(c)) c.childrenAccept(this);
      }
    }
    checkReadOnly();
    if(sgc==this) throw new SceneGraphLoopException();
    startWriter();
    try {
      sgc.childrenAccept(new CheckLoop());
    } catch(SceneGraphLoopException ex) {
      finishWriter();
      ex.fillInStackTrace(); throw ex;
    }
    if (children == Collections.EMPTY_LIST) children= new ArrayList<SceneGraphComponent>();
    int index=children.size();
    children.add(sgc);
    fireSceneGraphElementAdded(sgc, SceneGraphComponentEvent.CHILD_TYPE_COMPONENT, index);
    finishWriter();
  }

  /**
   * Use varargs in Java 5 to add multiple children at once
   * @param sgcList
   */
  public void addChildren(SceneGraphComponent ... sgcList)	{
	  startWriter();
	  try {
		  for (SceneGraphComponent sgc : sgcList)
			  addChild(sgc);
	  } finally {
		  finishWriter();
	  }
  }
  /**
  * Returns a child component node.
  * @return SceneGraphComponent
  */
  public SceneGraphComponent getChildComponent(int index) {
    startReader();
    try {
      return (SceneGraphComponent) children.get(index);
    } finally {
      finishReader();
    }
  }

  /**
   * returns a read-only list of the child component nodes.
   * @return a List containing the child component nodes
   */
  public List<SceneGraphComponent> getChildComponents() {
	  startReader();
	  try {
		  return Collections.unmodifiableList(children);
	  } finally {
		  finishReader();
	  }
  }
  /**
   * Returns the number of child component nodes.
   * @return int 
   */
  public int getChildComponentCount() {
    startReader();
    try {
      return children.size();
    } finally {
      finishReader();
    }
  }

	public boolean removeChild(SceneGraphComponent sgc) {
		checkReadOnly();
		startWriter();
		int index = children.indexOf(sgc);
		if (index == -1) {
			finishWriter();
			return false;
		}
		children.remove(index);
		fireSceneGraphElementRemoved(sgc,
				SceneGraphComponentEvent.CHILD_TYPE_COMPONENT, index);
		finishWriter();
		return true;
	}
  
	
	public void removeAllChildren() {
		checkReadOnly();
		while (getChildComponentCount() > 0) {
			SceneGraphComponent child = getChildComponent(0);
			removeChild(child);
		}
	}
  

  public void setAppearance(Appearance newApp) {
    checkReadOnly();
    startWriter();
    final Appearance old=appearance;
    appearance=newApp;
    fireSceneGraphElementSet(old, newApp, SceneGraphComponentEvent.CHILD_TYPE_APPEARANCE);
    finishWriter();
  }

  /**
   * Returns the appearance node.
   * @return Appearance
   */
  public Appearance getAppearance() {
    startReader();
    try {
      return appearance;
    } finally {
      finishReader();
    }
  }
  /**
   * Returns the camera child if any.
   * @return Camera
   */
  public Camera getCamera() {
    startReader();
    try {
      return camera;
    } finally {
      finishReader();
    }
  }
  /**
   * Adds a camera, replacing any previously added camera.
   * @param camera The camera to set
   */
  public void setCamera(Camera newCamera) {
    checkReadOnly();
    startWriter();
    final Camera old= camera;
    camera= newCamera;
    fireSceneGraphElementSet(old, newCamera, SceneGraphComponentEvent.CHILD_TYPE_CAMERA);
    finishWriter();
  }

  public Geometry getGeometry() {
    startReader();
    try {
      return geometry;
    } finally {
      finishReader();
    }
  }

  public void setGeometry(Geometry g) {
    checkReadOnly();
    startWriter();
    final Geometry old=geometry;
    geometry=g;
    fireSceneGraphElementSet(old, g, SceneGraphComponentEvent.CHILD_TYPE_GEOMETRY);
    finishWriter();
  }

  public AudioSource getAudioSource() {
	  startReader();
	  try {
		  return audioSource;
	  } finally {
		  finishReader();
	  }
  }

  public void setAudioSource(AudioSource a) {
	  checkReadOnly();
	  startWriter();
	  final AudioSource old=audioSource;
	  audioSource=a;
	  fireSceneGraphElementSet(old, a, SceneGraphComponentEvent.CHILD_TYPE_AUDIONODE);
	  finishWriter();
  }

  /**
   * Returns the light child if any.
   * @return Light
   */
  public Light getLight() {
    startReader();
    try {
      return light;
    } finally {
      finishReader();
    }
  }
  /**
  * Adds a light, replacing any previously added light.
  * @param light The light to set
  */
  public void setLight(Light newLight) {
    checkReadOnly();
    startWriter();
    final Light old=light;
    light= newLight;
    fireSceneGraphElementSet(old, newLight, SceneGraphComponentEvent.CHILD_TYPE_LIGHT);
    finishWriter();
  }

  /**
   * Returns the transformation node.
   * @return Transformation
   */
  public Transformation getTransformation() {
    startReader();
    try {
      return transformation;
    } finally {
      finishReader();
    }
  }

  public void setTransformation(Transformation newTrans) {
    checkReadOnly();
    startWriter();
    final Transformation oldTrans=transformation;
    transformation= newTrans;
    fireSceneGraphElementSet(oldTrans, newTrans, SceneGraphComponentEvent.CHILD_TYPE_TRANSFORMATION);
    finishWriter();
  }
  
  public boolean isDirectAncestor(SceneGraphNode child) {
    startReader();
    try {
      return transformation==child||appearance==child||
        camera==child||light==child||geometry==child||
        children.contains(child);
    } finally {
      finishReader();
    }
  }
  
//  public boolean isAncestor(final SceneGraphNode child) {
//    final boolean[] result={ false };
//    SceneGraphVisitor v=new SceneGraphVisitor()
//    {
//      public void visit(SceneGraphComponent c) {
//        if(!result[0]) c.childrenAccept(this);
//      }
//      public void visit(SceneGraphNode n) {
//        if(n==child) result[0]=true;
//      }
//    };
//    childrenAccept(v);
//    return result[0];
//  }
  
  public void accept(SceneGraphVisitor v) {
    startReader();
    try {
      v.visit(this);
    } finally {
      finishReader();
    }
  }
  static void superAccept(SceneGraphComponent c, SceneGraphVisitor v) {
    c.superAccept(v);
  }
  private void superAccept(SceneGraphVisitor v) {
    super.accept(v);
  }
  
  /**
   * This method calls the accept method on all childMembers in the following order
   * <ul>
   * <li> transformation
   * <li> appearance
   * <li> camera
   * <li> light
   * <li> geometry
   * <li> all child SceneGraphComponents
   * </ul>
   * The default use would be a rendering system, that implements the SceneGraphVisitor interface and 
   * calls <code>childrenAccept(this)</code> in its visit implementations.
   */
  public void childrenAccept(SceneGraphVisitor v) {
	  startReader();
	  try {
		  if(transformation != null) transformation.accept(v);
		  if(appearance != null)     appearance.accept(v);
		  if(camera != null)         camera.accept(v);
		  if(light != null)          light.accept(v);
		  if(geometry!=null)         geometry.accept(v);
		  if(audioSource!=null)        audioSource.accept(v);
		  for(SceneGraphComponent c: children) {
			  c.accept(v);
		  }
	  } finally {
		  finishReader();
	  }
  }

  public void childrenWriteAccept(SceneGraphVisitor v, boolean writeTransformation, boolean writeAppearance, boolean writeCamera, boolean writeLight, boolean writeGeometry, boolean writeChildren) {
	  childrenWriteAccept(v, writeTransformation, writeAppearance, writeCamera, writeLight, writeGeometry, writeChildren, false);
  }
  
  public void childrenWriteAccept(SceneGraphVisitor v, boolean writeTransformation, boolean writeAppearance, boolean writeCamera, boolean writeLight, boolean writeGeometry, boolean writeChildren, boolean writeAudio) {
    startReader();
    try {
      if(transformation != null) { 
        if (writeTransformation) transformation.startWriter();
        try {
          transformation.accept(v);
        } finally {
          if (writeTransformation) transformation.finishWriter();
        }
      }
      if(appearance != null) { 
        if (writeAppearance) appearance.startWriter();
        try {
          appearance.accept(v);
        } finally {
          if (writeAppearance) appearance.finishWriter();
        }
      }
      if(camera != null) { 
        if (writeCamera) camera.startWriter();
        try {
          camera.accept(v);
        } finally {
          if (writeCamera) camera.finishWriter();
        }
      }
      if(light != null) { 
        if (writeLight) light.startWriter();
        try {
          light.accept(v);
        } finally {
          if (writeLight) light.finishWriter();
        }
      }
      if(geometry != null) { 
        if (writeGeometry) geometry.startWriter();
        try {
          geometry.accept(v);
        } finally {
          if (writeGeometry) geometry.finishWriter();
        }
      }
      if(audioSource != null) { 
          if (writeAudio) audioSource.startWriter();
          try {
            audioSource.accept(v);
          } finally {
            if (writeAudio) audioSource.finishWriter();
          }
      }
      for(SceneGraphComponent c: children) {
    	  if (writeChildren) c.startWriter();
    	  try {
    		  c.accept(v);
    	  } finally {
    		  if (writeChildren) c.finishWriter();
    	  }
      }
    } finally {
      finishReader();
    }
  }

  /**
   *
   * Add a tool to this component. When the tool was added before, nothing happens.
   *
   * @param tool The tool to add.
   */
  public void addTool(Tool tool) {
    startWriter();
    try {
	    if(tools == Collections.EMPTY_LIST)
	    	tools= new ArrayList<Tool>();
	    if (tools.contains(tool)) return;//throw new IllegalStateException("duplicate tool");
	    tools.add(tool);
	    fireToolAdded(tool);
    } finally {
	    finishWriter();
    }
  }

  public boolean removeTool(Tool tool) {
    startWriter();
    int toolIndex= tools.indexOf(tool);
    if(toolIndex == -1) {
      finishWriter();
      return false;
    }
    tools.remove(toolIndex);
    fireToolRemoved(tool);
    finishWriter();
    return true;
  }
  
  private void fireToolAdded(Tool tool) {
    // cache event if there are listeners
    if (toolListener == null) return;
    final ToolEvent event = new ToolEvent(this, tool, ToolEvent.TOOL_ADDED);
    cachedEvents.add(event);
  }

  private void fireToolRemoved(Tool tool) {
    // cache event if there are listeners
    if (toolListener == null) return;
    final ToolEvent event = new ToolEvent(this, tool, ToolEvent.TOOL_REMOVED);
    cachedEvents.add(event);
  }

  /**
   * use from inside Scene.executeReader(..)
   * @return
   */
  public List<Tool> getTools() {
    startReader();
    try {
      return Collections.unmodifiableList(tools);
    } finally {
      finishReader();
    }
  }

   public void addToolListener(ToolListener listener) {
    startReader();
    toolListener.add(listener);
    finishReader();
  }
  
  public void removeToolListener(ToolListener listener) {
    startReader();
    toolListener.remove(listener);
    finishReader();
  }
  
  /**
   * @return a boolean that indicates wether this SceneGraphNode 
   * and its children get their geometry rendered or not.
   */
  public boolean isVisible() {
    startReader();
    try {
      return visible;
    } finally {
      finishReader();
    }
  }
  /**
   * Sets the visibility of this SceneGraphComponent and its children. This
   * flag affects rendering and bounding box calculations only (i.e. lights
   * and cameras at or below this node are unaffected).
   * @param visible sets wether this barnch of the scene graph should be rendered 
   * or not.
   */
  public void setVisible(boolean newVisibleState) {
    checkReadOnly();
    startWriter();
    if (visible != newVisibleState) {
      visible=newVisibleState;
      fireVisibilityChanged();
    }
    finishWriter();
  }

  /**
   * @return a boolean that indicates wether the geometry of this SceneGraphNode 
   * and its children can be picked.
   */
  public boolean isPickable() {
    startReader();
    try {
      return pickable;
    } finally {
      finishReader();
    }
  }
  /**
   * Sets the pickability of this SceneGraphComponent and its children. This
   * flag affects picking calculations only .
   * @param pickable sets wether this barnch of the scene graph should be pickable or not
   */
  public void setPickable(boolean newPickableState) {
    checkReadOnly();
    startWriter();
    if (pickable != newPickableState) {
      pickable=newPickableState;
      firePickabilityChanged();
    }
    finishWriter();
  }

  public void addSceneGraphComponentListener(SceneGraphComponentListener listener) {
    startReader();
    containerListener.add(listener);
    finishReader();
  }

  public void removeSceneGraphComponentListener(SceneGraphComponentListener listener) {
    startReader();
    containerListener.remove( listener);
    finishReader();
  }

  void fireSceneGraphElementAdded(final SceneGraphNode child, final int type, final int index) {
    // we are in write lock
    if (containerListener == null) return;
    final SceneGraphComponentEvent event = new SceneGraphComponentEvent(
          this, SceneGraphComponentEvent.EVENT_TYPE_ADDED,
          type, null, child, index);
    cachedEvents.add(event);
  }

  void fireSceneGraphElementRemoved(final SceneGraphNode child, final int type, final int index) {
    // we are in write lock
    if (containerListener == null) return;
    final SceneGraphComponentEvent event = new SceneGraphComponentEvent(this, SceneGraphComponentEvent.EVENT_TYPE_REMOVED, type, child, null, index);
    cachedEvents.add(event);
  }

  void fireSceneGraphElementReplaced(final SceneGraphNode old, final SceneGraphNode _new, final int type, final int index) {
    // we are in write lock
    if (containerListener == null) return;
    final SceneGraphComponentEvent event = new SceneGraphComponentEvent(this, SceneGraphComponentEvent.EVENT_TYPE_REPLACED, type, old, _new, index);
    cachedEvents.add(event);
  }

  void fireSceneGraphElementSet(final SceneGraphNode old, final SceneGraphNode _new, final int type) {
    // we are in write lock
    if(old==_new) return;
    if(old==null) fireSceneGraphElementAdded(_new, type, 0);
    else if(_new==null) fireSceneGraphElementRemoved(old, type, 0);
    else fireSceneGraphElementReplaced(old, _new, type, 0);
}

  void fireVisibilityChanged() {
    // we are in write lock
    if (containerListener == null) return;
    final SceneGraphComponentEvent event = 
    	new SceneGraphComponentEvent(this,SceneGraphComponentEvent.EVENT_TYPE_VISIBILITY_CHANGED);
    cachedEvents.add(event);
  }

  void firePickabilityChanged() {
	    if (containerListener == null) return;
	    final SceneGraphComponentEvent event = 
	    	new SceneGraphComponentEvent(this,SceneGraphComponentEvent.EVENT_TYPE_PICKABILITY_CHANGED);
	    cachedEvents.add(event);
	  }

  private void fire(SceneGraphComponentEvent event) {
    switch (event.getEventType()) {
    case SceneGraphComponentEvent.EVENT_TYPE_ADDED:
      containerListener.childAdded(event);
      return;
    case SceneGraphComponentEvent.EVENT_TYPE_REMOVED:
      containerListener.childRemoved(event);
      return;
    case SceneGraphComponentEvent.EVENT_TYPE_REPLACED:
      containerListener.childReplaced(event);
      return;
    case SceneGraphComponentEvent.EVENT_TYPE_VISIBILITY_CHANGED:
      containerListener.visibilityChanged(event);
    }
  }
  
  private void fire(ToolEvent event) {
    switch (event.getEventType()) {
    case ToolEvent.TOOL_ADDED:
      toolListener.toolAdded(event);
      return;
    case ToolEvent.TOOL_REMOVED:
      toolListener.toolRemoved(event);
      return;
    }
  }
  
  private void fire(SceneEvent event) {
    if (event instanceof ToolEvent) fire((ToolEvent) event);
    else fire((SceneGraphComponentEvent) event);
  }
  
  protected void writingFinished() {
    // we are in a readLock and broadcast all cached events - TODO: merge if possible
    try {
      for (SceneEvent event: cachedEvents) {
        // TODO: try catch ?
        fire(event);
      }
    } finally {
      cachedEvents.clear();
    }
  }
}
