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

import de.jreality.scene.SceneGraphPath;
import de.jreality.scene.Viewer;
import de.jreality.scene.data.DoubleArray;
import de.jreality.scene.pick.PickResult;
import de.jreality.scene.pick.PickSystem;

public interface ToolContext
{
    Viewer getViewer();
    
    /**
     * @return the source that triggers activation/perform/deactivate.
     */
    InputSlot getSource();
    
    DoubleArray getTransformationMatrix(InputSlot slot);
    AxisState getAxisState(InputSlot slot);
    
    /**
     * @return the time stamp of the event that's currently
     * being processed
     */
    long getTime();
    
    /**
     * @return Returns the path to the current tool if tool is not
     * activated by picking, path to pick otherwise
     */
    SceneGraphPath getRootToLocal();
    
    /**
     * @return Returns the path to the component where the
     * current tool is attatched
     */
    SceneGraphPath getRootToToolComponent();
    
    PickResult getCurrentPick();
    List<PickResult> getCurrentPicks();
    
    public SceneGraphPath getAvatarPath();
    
    public PickSystem getPickSystem();
    
    /**
     * a tool calls this method during activation
     * if the context is insufficient for activation.
     * That means the tool is not in activated state after
     * the activate call. calling this method at any other
     * time than activation, it has absolutely no effect.
     * 
     */
    public void reject();
    
    public Object getKey();
    
}
