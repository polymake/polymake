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

package de.jreality.openhaptics;

import java.awt.Color;

import de.jreality.geometry.Primitives;
import de.jreality.math.Matrix;
import de.jreality.math.MatrixBuilder;
import de.jreality.math.Pn;
import de.jreality.plugin.JRViewer;
import de.jreality.scene.Appearance;
import de.jreality.scene.Transformation;
import de.jreality.scene.proxy.scene.SceneGraphComponent;
import de.jreality.scene.tool.AbstractTool;
import de.jreality.scene.tool.InputSlot;
import de.jreality.scene.tool.ToolContext;
import de.jreality.shader.CommonAttributes;
import de.jreality.shader.HapticShader;
import de.jreality.shader.ShaderUtility;
import de.jreality.tools.DragEventTool;
import de.jreality.tools.FaceDragEvent;
import de.jreality.tools.FaceDragListener;
import de.jreality.tools.PointerDisplayTool;

public class HapticScene {
	static class TestTool extends AbstractTool {
		// mouse wheel up/down
		static final InputSlot trafo = InputSlot.getDevice("PointerShipTransformation");
		static final InputSlot button = InputSlot.getDevice("DragActivation");
		
		double minDistance = 0.1;
		double step = 0.1;
		
		public TestTool() {
			super(); // no activation axes, since this tool is always active
			addCurrentSlot(trafo, "Move camera away from map");
			addCurrentSlot(button, "Move camera toward map");
		}
		
		@Override
		public void perform(ToolContext tc) {
			System.out.println(tc.getAxisState(button));
		}
	}
	

	public static void main(String[] args) {
		SceneGraphComponent root = new SceneGraphComponent();
		de.jreality.scene.SceneGraphComponent child;
		
		int n = 2;
		double l = 4;
		for(int x =0; x <=n; x++){
			for(int y =0; y <=n; y++){
				for(int z =0; z <=n; z++){
					root.addChild(child = Primitives.sphere((double)l/n/3, new double[] { -l/2. + (double)x/n*l, 2. + (double)y/n*l, -l/2+(double)z/n*l}));
					child.setName("ball "+x+", "+y+","+z);
					child.setAppearance(new Appearance());
					HapticShader hs = ShaderUtility.createHapticShader(child.getAppearance());
					child.getAppearance().setAttribute(CommonAttributes.DIFFUSE_COLOR, new Color((float)x/n, (float)y/n, (float)z/n));
					hs.setStiffness(0.1+0.9*x/n);
					hs.setDamping((double)y/n);
					hs.setStaticFriction((double)z/n);
					hs.setDynamicFriction((double)z/n);
				}
			}
		}
		

		// root.addChild(Primitives.sphere(1, new double[]{0,0,0}));
		// root.addChild(Primitives.sphere(5, new double[]{0,0,0}));
		SceneGraphComponent sc = new SceneGraphComponent();
		sc.setGeometry(Primitives.box(5, 1, 5, false));
		sc.setTransformation(new Transformation());
		final Transformation trans = sc.getTransformation();

		sc.setAppearance(new Appearance());
		HapticShader hs = ShaderUtility.createHapticShader(sc.getAppearance());
		hs.setStaticFriction(0);

		
		DragEventTool t;
		sc.addTool(t = new DragEventTool());
		root.addChild(sc);
		root.addChild(new Trihedral("stiffness", "damping", "friction").translate(-l,1,-l));
		

		t.addFaceDragListener(new FaceDragListener() {
			public void faceDragged(FaceDragEvent e) {
				MatrixBuilder.init(new Matrix(trans.getMatrix()), Pn.EUCLIDEAN)
					.translate(e.getTranslation()).assignTo(trans);
			}
			public void faceDragStart(FaceDragEvent e) {
			}
			public void faceDragEnd(FaceDragEvent e) {
			}
		});

		//sc.addTool(new TestTool());
		root.addTool(new PointerDisplayTool());
		
		de.jreality.scene.Viewer v = JRViewer.display(root);

//
		do {
			v.renderAsync();
			try {
				Thread.sleep(30);
			} catch (InterruptedException e) {
				e.printStackTrace();
			}
		} while (true);
	}
}
