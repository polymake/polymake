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


package de.jreality.ui.viewerapp.actions.view;

import java.awt.Frame;
import java.awt.event.ActionEvent;
import java.lang.ref.WeakReference;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.List;

import de.jreality.ui.viewerapp.ViewerAppMenu;
import de.jreality.ui.viewerapp.actions.AbstractJrAction;


/**
 * Toggles full screen of the ViewerApp.<br>
 * There is only one instance of this action.
 * 
 * @author msommer
 */
public class Maximize extends AbstractJrAction {

  private boolean isFullscreen = false;
  private Frame frame;

  private static List<WeakReference<Maximize>> sharedInstances = new LinkedList<WeakReference<Maximize>>();
  
  
  private Maximize(String name, Frame frame) {
    super(name);
    this.frame = frame;
    
    setShortDescription("Maximize/Restore size of ViewerApp's frame");
  }

  
  /**
   * Returns a shared instance of this action depending on the specified frame
   * (i.e. there is a shared instance for each frame). 
   * The action's name is overwritten by the specified name.
   * @param name name of the action
   * @param frame the frame to toggle
   * @throws UnsupportedOperationException if frame equals null
   * @return shared instance of ToggleFullScreen with specified name
   */
  public static Maximize sharedInstance(String name, Frame frame) {
    if (frame == null) 
      throw new UnsupportedOperationException("Frame not allowed to be null!");
    
    Maximize sharedInstance = null;
    
    for (Iterator<WeakReference<Maximize>> it = sharedInstances.iterator(); it.hasNext(); ) {
    	WeakReference<Maximize> ref = it.next();
    	Maximize obj = ref.get();
    	if (obj == null) it.remove();
    	else if (obj.frame == frame) sharedInstance = obj;
    }
    

    if (sharedInstance == null) {
      sharedInstance = new Maximize(name, frame);
      sharedInstances.add(new WeakReference<Maximize>(sharedInstance));
    }
     
    sharedInstance.setName(name);
    return sharedInstance;
  }
  
  
  @Override
  public void actionPerformed(ActionEvent e) {
   
    if (isFullscreen) {  //exit full screen
      frame.dispose();
      frame.setUndecorated(false);
      frame.getGraphicsConfiguration().getDevice().setFullScreenWindow(null);
      setName(ViewerAppMenu.MAXIMIZE);
      frame.validate();
      frame.pack();  //reset to preferred sizes
      frame.setVisible(true);
      isFullscreen=false;
    } 
    else {  //switch to full screen
      frame.dispose();
      frame.setUndecorated(true);
      frame.getGraphicsConfiguration().getDevice().setFullScreenWindow(frame);
      setName(ViewerAppMenu.RESTORE);
      frame.validate();
      isFullscreen=true;
    }
  }
  
}