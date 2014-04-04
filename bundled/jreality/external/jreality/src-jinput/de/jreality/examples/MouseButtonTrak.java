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

/*
 * Created on 14.01.2006
 *
 * This file is part of the  package.
 * 
 * This program is free software; you can redistribute and/or modify 
 * it under the terms of the GNU General Public License as published 
 * by the Free Software Foundation; either version 2 of the license, or
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITTNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License 
 * along with this program; if not, write to the 
 * Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307
 * USA 
 */

public class MouseButtonTrak {

    private Controller mouse;
    private Component b1;
    private Component b2;
    private Component b3;
    
    public MouseButtonTrak() {
        ControllerEnvironment env = ControllerEnvironment.getDefaultEnvironment();
        Controller[] cts = env.getControllers();

        for(int i = 0; i< cts.length; i++) {
            String name = cts[i].getName();
            System.out.print(" controller "+i+"'s name is "+name);
            if(contains(name, "USB")&& contains(name, "Mouse")) {
                mouse = cts[i];
                System.out.println(" <- we use this one!");
            } 
            else System.out.println(".");
        }
        if(mouse == null)
            throw new RuntimeException("no USB mouse found!");
        mouse = mouse.getControllers()[1];
        Component[] a = mouse.getComponents(); 
        b1 = a[0];
        b2 = a[1];
        b3 = a[2];
        
    }

    private boolean contains(String name, String string) {
		return name.indexOf(string) != -1;
	}

    public final void poll() {
        mouse.poll();
    }
    
    public boolean pollButton1() {
        poll();
        return b1.getPollData()>0;
    }

    public boolean pollButton2() {
        poll();
        return b2.getPollData()>0;
    }

    public boolean pollButton3() {
        poll();
        return b3.getPollData()>0;
    }

    public boolean getButton1() {
        return b1.getPollData()>0;
    }

    public boolean getButton2() {
        return b2.getPollData()>0;
    }

    public boolean getButton3() {
        return b3.getPollData()>0;
    }

}
