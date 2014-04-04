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



/**
 * Timer Queue similar to javax.swing.TimerQueue. But this queue runs in the
 * ToolSystem Thread (in ToolEventQueue) and timers are performed also in this Thread.
 * 
 * @author Steffen Weissmann
 *
 */class TimerQueue implements AnimatorTask
{

    Timer   headTimer;
    
    long currentTime;

    public TimerQueue(AnimatorTool at) {
        at.schedule(this, this);
    }

    synchronized void addTimer(Timer timer, long expTime) {

    	if (timer.isRunning()) return;

        Timer prev = null;
        Timer next = headTimer;

        while (next != null) {
            if (next.expireTime > expTime) break;

            prev = next;
            next = next.next;
        }

        if (prev == null) headTimer = timer;
        else prev.next = timer;

        timer.expireTime = expTime;
        timer.next = next;
        timer.running = true;
    }


    synchronized void removeTimer(Timer timer) {
        Timer   previousTimer;
        Timer   nextTimer;
        boolean found;

        if (!timer.running) return;

        previousTimer = null;
        nextTimer = headTimer;
        found = false;

        while (nextTimer != null) {
            if (nextTimer == timer) {
                found = true;
                break;
            }

            previousTimer = nextTimer;
            nextTimer = nextTimer.next;
        }

        if (!found) return;

        if (previousTimer == null) {
            headTimer = timer.next;
        }
        else {
            previousTimer.next = timer.next;
        }

        timer.expireTime = 0;
        timer.next = null;
        timer.running = false;
    }


    synchronized boolean containsTimer(Timer timer) {
        return timer.running;
    }


    synchronized long processCurrentTimers() {
        Timer   timer;
        long    timeToWait;

        Timer myFirstTimer=null;
        do {
            timer = headTimer;
            if (timer == null) return 0;
            if (myFirstTimer == null) myFirstTimer=timer;
            else if (myFirstTimer == timer) return timer.expireTime - currentTime;
            
            timeToWait = timer.expireTime - currentTime;

            if (timeToWait <= 0) {
                timer.perform(currentTime);
                removeTimer(timer);
                if (timer.isRepeats()) addTimer(timer, currentTime + timer.getDelay());
            }
        } while (timeToWait <= 0);
        return timeToWait;
    }

    double nextRun=-1;
    
	public boolean run(double time, double dt) {
		currentTime=(long) time;
		if (nextRun != -1) {
			if (time < nextRun) return true;
		}
		long timeToWait = processCurrentTimers();
		nextRun = time+timeToWait;
		return true;
	}

	public void addTimer(Timer timer) {
		addTimer(timer, currentTime+timer.getInitialDelay());
	}


}
