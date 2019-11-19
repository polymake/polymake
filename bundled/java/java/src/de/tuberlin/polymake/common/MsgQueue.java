/* Copyright (c) 1997-2019
   Ewgenij Gawrilow, Michael Joswig, and the polymake team
   Technische Universität Berlin, Germany
   https://polymake.org

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2, or (at your option) any
   later version: http://www.gnu.org/licenses/gpl.txt.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
--------------------------------------------------------------------------------
*/

package de.tuberlin.polymake.common;

import java.util.Vector;

/**
 * This class implements a message queue of maximum length two.
 * If the queue already contains two elements the last one is overwritten.
 * 
 * @author Thilo Rörig
 */
public class MsgQueue {

    protected Vector queue = new Vector(2);

	/** create a new MsgQueue */
    public MsgQueue() {
    }

    /**
     * Get the first element of the queue.
     * 
     */
    public synchronized Object front() {
	return queue.firstElement();
    }

    /**
     * Remove the first element of the queue.
     */
    public synchronized void popFront() {
		queue.remove(0);
    }

    /**
     * If the size of the queue has reached two, the last element
     * is overwritten, otherwise, the element is appended to the queue.
     * @param obj object to be added to queue
     */
    public synchronized void pushBack(Object obj) {
		if (queue.size() == 2) {
		    queue.remove(1);
		    queue.add(1, obj);
		} else {
		    queue.addElement(obj);
		}
    }

	/**
	 * Tests if this MsgQueue is empty.
	 */
    public synchronized boolean isEmpty() {
		return queue.isEmpty();
    }

	/**
	 * Remove all elements of this MsgQueue.
	 */
    public synchronized void clear() {
		queue.clear();
    }
    
    /**
     * Returns a string representation of this MsgQueue containing
     * a string representation of all its elements.
     */
    public synchronized String toString() {
		return queue.toString();
    }
}
