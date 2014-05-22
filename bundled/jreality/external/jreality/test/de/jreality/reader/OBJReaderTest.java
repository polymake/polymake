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


package de.jreality.reader;

import java.net.URL;
import java.util.List;
import java.util.logging.Level;

import junit.framework.TestCase;
import de.jreality.scene.Appearance;
import de.jreality.scene.Geometry;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.data.Attribute;
import de.jreality.util.Input;
import de.jreality.util.LoggingSystem;


/**
 *
 * TODO: comment this
 *
 * @author weissman
 *
 */
public class OBJReaderTest extends TestCase {

    public static void main(String[] args) {
        junit.textui.TestRunner.run(OBJReaderTest.class);
    }
	
	public void setUp() {
       LoggingSystem.getLogger(ParserMTL.class).setLevel(Level.OFF);
	}

	public void tearDown() {
       LoggingSystem.getLogger(ParserMTL.class).setLevel(null);
	}

    public void testOBJReader() throws Exception {
        //String fileName = "/home/gollwas/bolt1.obj";
        //String fileName = "/home/gollwas/cube2.obj";
        URL url = this.getClass().getResource("square.obj");
        SceneGraphComponent sgc = new ReaderOBJ().read(url); 
        assertEquals("sgc 0", sgc.getName());
        assertEquals("[len=4 storage=double[][3]]", 
        	sgc.getChildComponent(0).getGeometry().getAttributes(Geometry.CATEGORY_VERTEX, Attribute.COORDINATES).toString());
    }
    
//    public void test3DSReader() throws Exception {
//        String fileName = "/home/gollwas/3ds/tetranoid_0_7.3ds";
//        SceneGraphComponent sgc = new Reader3DS().read(new File(fileName)); 
//    }

    public void testMTLReader() throws Exception {
        URL url = this.getClass().getResource("vp.mtl");
        List<?> list = ParserMTL.readAppearences(Input.getInput(url));
        assertEquals("baerFinal", ((Appearance) list.get(0)).getName());
    }

}
