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


package de.jreality.examples;

import net.java.games.input.Component;
import net.java.games.input.Controller;
import net.java.games.input.ControllerEnvironment;

public class GameTrak {

    private Controller gameTrak;
    private Component phi1;
    private Component phi2;
    private Component theta1;
    private Component theta2;
    private Component height1;
    private Component height2;
    public GameTrak() {
        ControllerEnvironment env = ControllerEnvironment.getDefaultEnvironment();
        Controller[] cts = env.getControllers();
        
        for(int i = 0; i< cts.length; i++) {
            String name = cts[i].getName();
            System.out.print(" controller "+i+"'s name is "+name);
            if(contains(name, "Game-Trak")) {
                gameTrak = cts[i];
                System.out.println(" <- we use this one!");
            } 
            else System.out.println(".");
        }
        if(gameTrak == null)
            throw new RuntimeException("no Game-Trak found!");
        Component[] a = gameTrak.getComponents(); 
        phi1 = a[0];
        theta1 = a[1];
        height1 = a[2];

        phi2 = a[3];
        theta2 = a[4];
        height2 = a[5];
        
}

    private boolean contains(String name, String string) {
		return name.indexOf(string) != -1;
	}

	public void poll() {
        gameTrak.poll();
    }
    
    public float getPhi1() {
        return phi1.getPollData();
    }

    public float getPhi2() {
        return phi2.getPollData();
    }

    public float getTheta1() {
        return theta1.getPollData();
    }
    public float getTheta2() {
        return theta2.getPollData();
    }

    public float getHeight1() {
        return height1.getPollData();
    }
    public float getHeight2() {
        return height2.getPollData();
    }
    
    /**
     * first (left) point.
     * @param p
     */
    private static final float DISPLACEMENT = .05f;
    public final void getPoint1(float[] p) {
        float phi = (float)( (getPhi1())*Math.PI/4.f);
        float theta = (float)( (getTheta1())*Math.PI/4.f);
        float h = 1-getHeight1();
        p[0] =  - DISPLACEMENT+(float) (h*Math.sin(phi)*Math.cos(theta));
        p[1] = (float) (h*Math.cos(phi)*Math.cos(theta));
        p[2] = (float) (-h*Math.sin(theta));
        
    }
    /**
     * second (right) point.
     * @param p
     */
    public final void getPoint2(float[] p) {
        float phi = (float)( (getPhi2())*Math.PI/4.f);
        float theta = (float)( (getTheta2())*Math.PI/4.f);
        float h = 1-getHeight2();
        p[0] =  DISPLACEMENT +(float) (h*Math.sin(phi)*Math.cos(theta));
        p[1] = (float) (h*Math.cos(phi)*Math.cos(theta));
        p[2] = (float) (-h*Math.sin(theta));
    }
}
