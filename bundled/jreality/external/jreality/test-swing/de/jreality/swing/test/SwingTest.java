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


package de.jreality.swing.test;

import javax.swing.JButton;

import de.jreality.examples.CatenoidHelicoid;
import de.jreality.examples.PaintComponent;
import de.jreality.plugin.JRViewer;
import de.jreality.plugin.content.ContentAppearance;
import de.jreality.plugin.content.ContentLoader;
import de.jreality.plugin.content.ContentTools;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.Transformation;
import de.jreality.swing.JFakeFrame;
public class SwingTest {


    /**
     * @param args
     */
    public static void main(String[] args) {
       PaintComponent pc = new PaintComponent();

       CatenoidHelicoid catenoid = new CatenoidHelicoid(50);
       catenoid.setAlpha(Math.PI/2.-0.3);
       
       SceneGraphComponent catComp= new SceneGraphComponent();
       Transformation gt= new Transformation();

       catComp.setTransformation(gt);
       catComp.setGeometry(catenoid);
       SceneGraphComponent c;
//    try {
//        c = de.jreality.reader.Readers.read(new File(args[0]));
//       catComp.setGeometry(c.getChildComponent(0).getGeometry());
//    } catch (IOException e) {
//        // TODO Auto-generated catch block
//        e.printStackTrace();
//    }
//       try {
//        catComp = 
//               de.jreality.reader.Readers.read(new File("/home/timh/jRdata/testData3D/obj/baer10000_punched_vt.obj"));
//    } catch (IOException e) {
//        e.printStackTrace();
//    }
//        catComp.getChildComponent(0).setAppearance(null);
       // AABBPickSystem does this authomatically now:
       //PickUtility.assignFaceAABBTree(catenoid);
   
       
       JFakeFrame jrj = new JFakeFrame();
       jrj.getContentPane().add(new JButton("my button"));
       jrj.setSize(200, 200);
       jrj.setVisible(true);
       catComp.addTool(jrj.getTool());
       
    System.out.print("setting appearance ");
       catComp.setAppearance(jrj.getAppearance());
       System.out.println("done");
	    JRViewer v = new JRViewer();
		v.addBasicUI();
		v.setContent(catComp);
		v.registerPlugin(new ContentAppearance());
		v.registerPlugin(new ContentLoader());
		v.registerPlugin(new ContentTools());
		v.startup();       
    }

}
