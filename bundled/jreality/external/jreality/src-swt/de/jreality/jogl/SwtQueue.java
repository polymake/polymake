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


package de.jreality.jogl;

import java.util.LinkedList;

import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Shell;

public class SwtQueue implements Runnable {

  private static SwtQueue instance = new SwtQueue();
  
  private Display display;
  
  private boolean inited = false;
  private final Object initLock=new Object();
  
  private final Thread swtThread;
  
  public static SwtQueue getInstance() {
    return instance;
  }
  
  private SwtQueue() {
    swtThread = new Thread(this);
    swtThread.start();
    synchronized(initLock) {
      try {
        while (!inited) initLock.wait();
      } catch (InterruptedException e) {
        // TODO Auto-generated catch block
        e.printStackTrace();
      }
    }
  }
  
  public Display getDisplay() {
    return display;
  }

  int shellCnt=0;
  
  public Shell createShell() {
    final Shell[] shell = new Shell[1];
    Runnable r = new Runnable() {
      public void run() {
        shell[0] = new Shell(display);
        shellCnt++;
        shell[0].addDisposeListener(new DisposeListener() {
          public void widgetDisposed(DisposeEvent arg0) {
            shellCnt--;
            if (shellCnt==0) display.dispose();
          }
        });
      };
    };
    waitFor(r);
    return (shell[0]);
  }

  public void run() {
    synchronized (initLock) {
      display = new Display();
      inited = true;
      initLock.notify();
    }
    Runnable task = null;
    while (!display.isDisposed()) {
      while (true) {
        synchronized (tasks) {
          if (tasks.size() > 0)
            task = (Runnable) tasks.removeFirst();
        }
        if (task != null) { task.run(); task=null; }
        else break;
      }
      if (!display.readAndDispatch()) {
        display.sleep();
      }
    }
    display.dispose();
    System.exit(0);
  }
  
  private LinkedList tasks = new LinkedList();
  
  public void invokeLater(Runnable r) {
    synchronized (tasks) {
      tasks.addLast(r);
      display.wake();
    }
  }
   
  public void waitFor(Runnable r) {
    if (Thread.currentThread() == swtThread) r.run();
    else {
      TrackedRunnable rr = new TrackedRunnable(r);
      synchronized (tasks) {
        tasks.addLast(rr);
        display.wake();
      }
      rr.waitFor();
    }
  }
  
  private static final class TrackedRunnable implements Runnable {

    private final Runnable task;
    private boolean done=false;
    
    TrackedRunnable(Runnable task) {
      this.task = task;
    }
    
    public void run() {
      task.run();
      synchronized (task) {
        done=true;
        task.notify();
      }
    }
    
    void waitFor() {
      synchronized (task) {
        while (!done) {
          try {
            task.wait();
          } catch (InterruptedException e) {
            throw new Error();
          }
        }
      }
    }
    
  }
}
