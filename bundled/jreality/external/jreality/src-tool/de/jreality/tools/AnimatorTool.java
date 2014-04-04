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

import java.util.HashMap;
import java.util.IdentityHashMap;
import java.util.Iterator;

import de.jreality.scene.tool.AbstractTool;
import de.jreality.scene.tool.InputSlot;
import de.jreality.scene.tool.ToolContext;
import de.jreality.toolsystem.ToolSystem;


/**
 *
 * TODO: comment this
 *
 * @author weissman
 *
 */
public class AnimatorTool extends AbstractTool {

	private static InputSlot timer = InputSlot.getDevice("SystemTime");

	private static HashMap<Object, AnimatorTool> instances=new HashMap<Object, AnimatorTool>();

	public static AnimatorTool getInstance(ToolContext context) {
		return getInstanceImpl((Object) context.getKey());
	}

	/**
	 * WARNING: do not use this unless you write a tool system!!
	 */
	public static AnimatorTool getInstanceImpl(Object key) {
		//if (!thread.getName().equals("jReality ToolSystem EventQueue"))
		//    throw new RuntimeException("no tool system event thread!");
		AnimatorTool instance = (AnimatorTool) instances.get(key);
		if (instance == null) {
			instance = new AnimatorTool();
			instances.put(key, instance);
		}
		return instance;
	}

	/**
	 * WARNING: do not use this unless you write a tool system!!
	 */
	public static void disposeInstance(Object key) {
		AnimatorTool at = instances.remove(key);
		at.dispose();
	}

	private TimerQueue timerQueue;

	private IdentityHashMap<Object, AnimatorTask> animators = new IdentityHashMap<Object, AnimatorTask>();
	private final Object mutex = new Object();

	private double totalTime;

	private AnimatorTool() {
		addCurrentSlot(timer, "Triggers the animator tasks.");
		timerQueue = new TimerQueue(this);
	}

	public void perform(ToolContext tc) {
		synchronized (mutex) {
			int dt = tc.getAxisState(timer).intValue();
			totalTime+=dt;
			for (Iterator<AnimatorTask> i = animators.values().iterator(); i.hasNext(); ) {
				AnimatorTask task = i.next();
				if (!task.run(totalTime, dt)) {
					i.remove();
				}
			}
		}
	}

	public void schedule(Object key, AnimatorTask task) {
		synchronized (mutex) {
			animators.put(key, task);
		}
	}

	public void deschedule(Object key) {
		synchronized (mutex) {
			animators.remove(key);
		}
	}

	public TimerQueue getTimerQueue() {
		return timerQueue;
	}

	public void dispose() {
		synchronized (mutex) {
			animators.clear();
		}
	}

	public static AnimatorTool getInstance(ToolSystem ts) {
		return getInstanceImpl(ts.getKey());
	}

}
