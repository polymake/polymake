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


package de.jreality.toolsystem;

import java.util.LinkedList;
import java.util.ListIterator;
import java.util.logging.Level;

import de.jreality.scene.tool.InputSlot;
import de.jreality.util.LoggingSystem;

/**
 * 
 * TODO: check sync!
 * 
 * @author weissman
 *
 **/
public class ToolEventQueue {
    
    private ToolEventReceiver receiver;
    private LinkedList<ToolEvent> queue = new LinkedList<ToolEvent>();
    private final Object mutex = new Object();
    protected volatile boolean running = true;
    
    private Runnable eventThread = new Runnable() {
    	public void run() {
            ToolEvent event;
            boolean isRunning;
            loop: while (true) {
                synchronized(mutex) {
                    while (queue.isEmpty()) {
                        try {
                        	if (running) mutex.wait();
                            else break loop;
                        } catch (InterruptedException e) {
                            throw new Error();
                        }
                    }
                    event = (ToolEvent) queue.removeFirst();
                    isRunning = running;
                }
                if (isRunning) try {
                	receiver.processToolEvent(event);
                } catch (Throwable e) {
                	e.printStackTrace();
                }
                else break;
            }
            System.out.println("TEQ shut down.");
        }    
    };
    
    public ToolEventQueue(ToolEventReceiver receiver) {
        this.receiver = receiver;
    }

    private volatile boolean started = false;
    private Thread thread = new Thread(eventThread);
    {
      thread.setName("jReality ToolSystem EventQueue");
    }
    public void start() {
      if (started) throw new IllegalStateException("already started");
      started = true;
      thread.start();
    }
    
    /**
     * places the given event into the queue
     * if queue was started already
     * 
     * @param event the event to post
     * @return true if the event was added false if not
     */
    public boolean addEvent(ToolEvent event) {
      if (!started) return false;
      placeEvent(event);
      return true;
    }
    
    /**
     * returns wether the event was added or if it replaced another event
     * @param event
     * @param senderWaits
     * @return true if the event was added to the queue, false if replaced an
     * already scheduled event
     */
    private boolean placeEvent(ToolEvent event) {
      synchronized(mutex) {
    	if (!running) return false;
        // we replace the last possible event
        for (ListIterator i = queue.listIterator(queue.size()); i.hasPrevious(); ) {
            ToolEvent e = (ToolEvent) i.previous();
            if (event.canReplace(e)) {
                LoggingSystem.getLogger(this).log(e.getInputSlot() == InputSlot.getDevice("SystemTime") ? Level.FINEST:Level.FINER, "replacing ToolEvent {0} with {1}", new Object[]{e, event});
                e.replaceWith(event);
                return false;
            }
        }
        queue.addLast(event);
//        System.out.println(queue);
        mutex.notify();
      }
      return true;
    }
    
    public void dispose() {
        synchronized (mutex) {
	        running = false;
            mutex.notifyAll();
            queue.clear();
        }
    }

    public Thread getThread() {
      return thread;
    }
}