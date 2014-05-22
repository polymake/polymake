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


package de.jreality.tools;

import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;

import de.jreality.scene.tool.AbstractTool;
import de.jreality.scene.tool.InputSlot;
import de.jreality.scene.tool.ToolContext;

/**
 * 
 * Tool that generates an {@link java.awt.event.ActionEvent} on activation.
 * This can be used to trigger arbitrary things when a geometry is picked.
 * 
 * @author Ulrich
 */

public class ActionTool extends AbstractTool {

    private int numListener;
    private ActionListener[] listener;
    private final Object mutex=new Object();
    
    public ActionTool(InputSlot activationSlot) {
      super(activationSlot);
      numListener=0;
      listener = new ActionListener[10];
    }
    
    public ActionTool(String activationSlotName) {
      this(InputSlot.getDevice(activationSlotName));
    }
    
    public void activate(ToolContext tc) {
      fire(tc);
    }
    
    public void fire(Object obj) {
      synchronized(mutex) {
        ActionEvent ev = new ActionEvent(obj,0,"ActionTool");
        for(int i=0; i<numListener; i++) {
          listener[i].actionPerformed(ev);
        }
      }
    }
    public void addActionListener(ActionListener l) {
      synchronized(mutex) {
        if(numListener==listener.length) {
          ActionListener[] newListener=new ActionListener[numListener+10];
          System.arraycopy(listener,0,newListener,0,numListener);
          listener=newListener;
        }
        listener[numListener++]=l;
      }
    }
    
    public void removeActionListener(ActionListener l) {
      synchronized(mutex) {
        int i;
        find: {
          for(i=0; i<numListener; i++)
            if(listener[i]==l) break find;
          return;
        }
        numListener--;
        if(i!=numListener) {
          System.arraycopy(listener,i+1,listener,i,numListener-i);
        }
        listener[numListener]=null;
      }
    }
}
