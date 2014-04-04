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


package de.jreality.scene.tool;

import java.util.List;

/**
 * <p>
 * Tools are attatched to a SceneGraphComponent and are intended to
 * to perform interactive changes to the scene - usually driven by
 * user input. The corresponding methods are 
 * {@link #activate(ToolContext)},
 * {@link #perform(ToolContext)} and {@link #deactivate(ToolContext)}.
 * </p>
 * <p>
 * User input is passed to the Tool either as an {@link AxisState}, which represents
 * a double value (i.e. a mouse button position) or a DoubleArray of length 16, which
 * represents a 4 by 4 matrix. This matrix typically represents a <i>euclidean</i>
 * isometry that represents the original user input converted into a suitable coordinate
 * system.  (Tool writers for non-euclidean settings need to be careful to convert
 * these matrices if necessary.) 
 * <p>
 * These inputs are called <i>virtual devices</i>,
 * since they are usually hardware independent and "live in the scene".
 * These virtual devices are mapped to {@link de.jreality.scene.tool.InputSlot}s,
 * which should represent them under a meaningful name. Some examples (which
 * are available in the default configuration file <i>toolconfig.xml</i>):
 * <ul>
 * <li><code>"PointerTransformation"</code> A pointer device in scene coordinates.
 *     On a desktop this represents the mouse pointer as a free vector whose base
 *     point is on the near clipping plane, and whose direction points away from the camera;
 *     in a traditional immersive environment this would be the actual 3D position and
 *     direction of the wand. Both are represented by 4x4 matrices as indicated above. 
 *     The direction of the pointer is the -Z axis.[steffen: is this last sentence correct, or only
 *     for non-perspective cameras?]</li>
 * <li><code>"PrimaryActivation"</code> An axis state used for default interaction
 *     with the scene. On a desktop per default it is the left mouse button;
 *     in the Portal the left wand button.</li>
 * <li><code>"SystemTime"</code> is an axis state which is permanently triggered.
 *     The intValue() of its AxisState gives the time in milli-seconds since the
 *     last emission of the SystemTime device.</li>
 * <li>TODO... </li>
 * </ul>
 * </p>
 * <p>
 * Tools may be always active or activated by some virtual device.
 * A Tool which is not always active ({@link getActivationSlot()} returns
 * not null) will be activated as soon as one of its activation slots reaches
 * the state AxisState.PRESSED. <b>Warning</b>: If the activation slot does not
 * represent an AxisState, the tool will never become active. 
 * </p>
 * <p>
 * A single Tool instance can be attached to different scene graph components. A tool
 * attached to a scene graph component that appears at multiple positions in the scene graph
 * will also, implicitly, be instanced multiple times; each instance will have its own local state
 * not shared with other instances. [steffen: is this right?] The current path is
 * always available via the ToolContext: {@link de.jreality.scene.tool.ToolContext.getRootToLocal()}
 * and {@link de.jreality.scene.tool.ToolContext.getRootToToolComponent()}
 * return the paths for the current {@link #activate(ToolContext)}/{@link #perform(ToolContext)}/
 * {@link #deactivate(ToolContext)} call.
 * </p>
 *  
 * 
 * @author Steffen Weissmann
 *
 */
public interface Tool {

  /**
   * 
   * If the result is empty, then the tool is always active.
   * 
   * If the result is not empty, then the tool becomes
   * active as soon as the axis of one activation slot is pressed.
   * This implies that the {@link InputSlot}s must be associated to
   * {@link AxisState}s, otherwise the Tool will never become active.
   * <br>The tool gets deactivated, as soon as the InputSlot that
   * caused activation changes its state to {@link AxisState.RELEASED}.
   * <br> When the tool is active, other activation axes are ignored
   * and passed to other {@link Tool}s down the path.
   * 
   * The result must remain constant.
   * 
   * @return List of InputSlots for activating the tool
   */
	List<InputSlot> getActivationSlots();

  /**
   * This method will only be called for active tools. The
   * currentSlots may change after each call of <code>activate(..)</code>
   * or <code>perform(..)</code>.
   * 
   * @return list of currently relevant input slots
   */
  List<InputSlot> getCurrentSlots();

  /**
   * This method is called when the tool gets activated. Note that
   * it will never be called if the tool is always active.
   * 
   * @param tc The current tool context
   */
  void activate(ToolContext tc);

  /**
   * This method is called when the tool is activated and any
   * AxisState or TransformationMatrix of the current slots changes.
   * 
   * @param tc The current tool context
   */
  void perform(ToolContext tc);

  /**
   * this method is called when the tool was activate and the
   * AxisState of the activation slot changes to AxisState.RELEASED - to zero. 
   * Note that it will never be called for always active tools.
   * 
   * @param tc The current tool context
   */
  void deactivate(ToolContext tc);

  /**
   * Gives a description of the meaning of the given InputSlot.
   * This may depend on the current state of the Tool.
   * 
   * @param slot to describe
   * @return A description of the current meaning of the given
   * InputSlot. 
   */
  String getDescription(InputSlot slot);
  
  /**
   * Gives an overall description of this Tool.
   * 
   * @return A description of the Tool including information
   * about activation and overall behaviour.
   */
  String getDescription();
}