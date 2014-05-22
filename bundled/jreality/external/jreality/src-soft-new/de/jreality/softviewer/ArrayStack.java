/*
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

package de.jreality.softviewer;



public class ArrayStack {
    private Triangle[] data;
    int position = -1;
    private final int increment;
    public ArrayStack(int initialCapacity) {
        super();
        data = new Triangle[initialCapacity];
        increment = initialCapacity;
    }
    public boolean isEmpty() {
        return position == -1;
    }
    public int getSize() {
        return position+1;
    }
    /**
     * pop allways returns a triangle. If the stack is empty a new one is generated
     * @return a triangle
     */
    public Triangle pop() {
        if(position > -1) { 
            Triangle result =  data[position];
        data[position--] = null;
        return result;
        } else 
            return new Triangle();
    }
    public Triangle peek() {
        return data[position];
    }
    public void push(Triangle element) {
        if(++position >=data.length) {
            Triangle[] tmp = new Triangle[data.length + increment];
            System.arraycopy(data,0,tmp,0,data.length);
            data = tmp;
        }
        data[position] = element;
            
    }
    public Triangle[] getArray() {
        return data;
    }
    public void set(int i, Triangle e) {
        data[i] = e;
    }
    public Triangle get(int i) {
        return data[i];
    }
}
