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

import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.tool.AbstractTool;
import de.jreality.scene.tool.InputSlot;
import de.jreality.scene.tool.ToolContext;
import de.jreality.toolsystem.ToolSystem;

/**
 * This is a drop-in replacement for javax.swing.Timer, when using in jreality.
 * Note that both timer queueing and the timer tasks are running in the tool
 * system thread - to perform gui tasks you need to make sure that the action
 * is performed in the EventDispatchThread.
 * <p>
 * The coalesce flag has no influence for this timer, since both queueing and execution
 * happens in the same thread. Therefore, coalescing is implicitly true.
 * 
 * <B>NOTE:</B> The timer will not do anything unless you call ToolUtility.attachTimer(...)!
 * 
 * @author Steffen Weissmann
 *
 */
@SuppressWarnings("serial")
public class Timer extends javax.swing.Timer {

    // These fields are maintained by TimerQueue.
    long    expireTime;
    Timer   next;
    boolean running;
	
	TimerQueue timerQueue;
	
	public Timer(int delay, ActionListener listener) {
		super(delay, listener);
	}
	
//	public void attach(ToolSystemViewer tsv) {
//		attach(tsv.getToolSystem());
//	}
//
	public void attach(ToolSystem ts) {
		attach(AnimatorTool.getInstance(ts));
	}
	
	public void attach(ToolContext tc) {
		attach(AnimatorTool.getInstance(tc));
	}

	public void attach(AnimatorTool at) {
		if (timerQueue != null) return;
		timerQueue = at.getTimerQueue();
	}
	
	public void attach(SceneGraphComponent cmp) {
		if (timerQueue != null) return;
		cmp.addTool(new AbstractTool() {
			{
				addCurrentSlot(InputSlot.getDevice("SystemTime"));
			}
			@Override
			public void perform(ToolContext tc) {
				attach(tc);
				tc.getRootToToolComponent().getLastComponent().removeTool(this);
			}
		});
	}

    TimerQueue timerQueue() {
    	return timerQueue;
    }
    
    /**
     * Starts the <code>Timer</code>,
     * causing it to start sending action events
     * to its listeners.
     *
     * @see #stop
     */
    public void start() {
    	if (timerQueue == null) throw new IllegalStateException("timer not attached!");
        timerQueue().addTimer(this);
    }


    /**
     * Returns <code>true</code> if the <code>Timer</code> is running.
     *
     * @see #start
     */
    public boolean isRunning() {
        return timerQueue != null && timerQueue().containsTimer(this);
    }


    /**
     * Stops the <code>Timer</code>,
     * causing it to stop sending action events
     * to its listeners.
     *
     * @see #start
     */
    public void stop() {
        timerQueue().removeTimer(this);
    }

    synchronized void perform(long currentTime) {
    	fireActionPerformed(new ActionEvent(this, 0, null,
                currentTime,
                0));
    }

}
