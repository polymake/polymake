package de.jreality.ui;

import javax.media.opengl.DefaultGLCapabilitiesChooser;
import javax.media.opengl.GLCapabilities;
import javax.media.opengl.GLCapabilitiesChooser;
import javax.media.opengl.GLProfile;
import javax.media.opengl.awt.GLCanvas;
import javax.swing.JFrame;

public class Display1Test {

	
	
	public static void main(String[] args) {
		GLCapabilitiesChooser capChooser = new DefaultGLCapabilitiesChooser();
		GLCapabilities caps = new GLCapabilities(GLProfile.get("GL2"));
		System.out.println("using caps: " + caps);
		GLCanvas canvas = new GLCanvas(caps, capChooser, null, null);
		
		JFrame f = new JFrame("Display 1 Test");
		f.setSize(1024, 600);
		f.add(canvas);
		f.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
		f.setVisible(true);
	}
	
}
