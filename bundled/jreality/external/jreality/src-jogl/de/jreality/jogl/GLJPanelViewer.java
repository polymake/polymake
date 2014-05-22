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

import java.awt.Graphics;
import java.awt.Graphics2D;
import java.lang.ref.WeakReference;
import java.util.EventObject;
import java.util.Vector;
import java.util.logging.Level;

import javax.media.opengl.DefaultGLCapabilitiesChooser;
import javax.media.opengl.GLAutoDrawable;
import javax.media.opengl.GLCapabilities;
import javax.media.opengl.GLCapabilitiesChooser;
import javax.media.opengl.GLContext;
import javax.media.opengl.GLDrawable;
import javax.media.opengl.GLDrawableFactory;
import javax.media.opengl.GLPbuffer;
import javax.media.opengl.GLProfile;
import javax.media.opengl.awt.GLJPanel;

import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SceneGraphPath;
import de.jreality.util.SceneGraphUtility;

public class GLJPanelViewer extends AbstractViewer {
	GLJPanel panel;
	GLPbuffer sharedPBuffer;
	boolean opaque = false;
	transient boolean preRender = true;
	transient Graphics2D g2d;

	public GLJPanelViewer() {
		this(null, null);
	}

	public GLJPanelViewer(SceneGraphPath camPath, SceneGraphComponent root) {
		setAuxiliaryRoot(SceneGraphUtility
				.createFullSceneGraphComponent("AuxiliaryRoot"));
		initializeFrom(root, camPath);
		panel.setOpaque(opaque);
	}

	// override these methods as subclass to draw beneath (above) the jReality
	// scene
	public void paintBefore(Graphics g) {
		preRender = true;
		g2d = (Graphics2D) g;
		broadcastChange();
		// getSceneRoot().getAppearance().setAttribute("backgroundColor", new
		// Color(0,255,0,128));
		// g2.setColor(Color.blue);
		// g2.fillRect(0, 50, 50, 50);
	}

	public void paintAfter(Graphics g) {
		preRender = false;
		g2d = (Graphics2D) g;
		broadcastChange();
		// Graphics2D g2 = (Graphics2D) g;
		// g2.setColor(Color.pink);
		// g2.fillRect(0, 0, 50, 50);
	}

	@Override
	protected void initializeFrom(SceneGraphComponent r, SceneGraphPath p) {
		setSceneRoot(r);
		setCameraPath(p);
		GLCapabilities caps = new GLCapabilities(GLProfile.get("GL2"));
		caps.setAlphaBits(8);
		caps.setStereo(JOGLConfiguration.quadBufferedStereo);
		caps.setDoubleBuffered(true);
		GLCapabilitiesChooser chooser = new MultisampleChooser();

		GLContext sharedContext = firstOne.get();

		if (JOGLConfiguration.multiSample) {
			caps.setSampleBuffers(true);
			caps.setNumSamples(4);
			caps.setStereo(JOGLConfiguration.quadBufferedStereo);
		} else {
			chooser = new DefaultGLCapabilitiesChooser();
		}
		if (JOGLConfiguration.sharedContexts && sharedContext == null)
			setupSharedContext(caps, chooser, sharedContext.getGLDrawable());

		panel = new GLJPanel(caps, chooser, sharedContext) {

			@Override
			protected void paintComponent(Graphics arg0) {
				paintBefore(arg0);
				super.paintComponent(arg0);
				paintAfter(arg0);
			}

		};
		drawable = panel;
		JOGLConfiguration.getLogger().log(Level.INFO,
				"Caps is " + caps.toString());
		drawable.addGLEventListener(this);
		if (JOGLConfiguration.quadBufferedStereo)
			setStereoType(HARDWARE_BUFFER_STEREO);
		// panel.updateUI();
	}

	// have to use a pbuffer to start with since panel has no context until
	// it's visible.
	private void setupSharedContext(GLCapabilities caps,
			GLCapabilitiesChooser chooser, GLDrawable glDrawable) {
		if (sharedPBuffer == null)
			sharedPBuffer = GLDrawableFactory.getFactory(GLProfile.get("GL2"))
					.createGLPbuffer(
							glDrawable.getNativeSurface()
									.getGraphicsConfiguration().getScreen()
									.getDevice(), caps, chooser, 1, 1, null);
		firstOne = new WeakReference<GLContext>(sharedPBuffer.getContext());
	}

	public boolean isOpaque() {
		return opaque;
	}

	public void setOpaque(boolean opaque) {
		this.opaque = opaque;
		panel.setOpaque(opaque);
	}

	Vector<GLJPanelListener> panelListeners;

	public interface GLJPanelListener extends java.util.EventListener {
		public void preRender(Graphics2D g2);

		public void postRender(Graphics2D g2);
	}

	public void addRenderListener(GLJPanelListener l) {
		if (panelListeners == null)
			panelListeners = new Vector<GLJPanelListener>();
		if (panelListeners.contains(l))
			return;
		panelListeners.add(l);
		// JOGLConfiguration.theLog.log(Level.INFO,"Viewer: Adding geometry listener"+l+"to this:"+this);
	}

	public void removeRenderListener(GLJPanelListener l) {
		if (panelListeners == null)
			return;
		panelListeners.remove(l);
	}

	public void broadcastChange() {
		if (panelListeners == null)
			return;
		// SyJOGLConfiguration.theLog.log(Level.INFO,"Viewer: broadcasting"+listeners.size()+" listeners");
		if (!panelListeners.isEmpty()) {
			EventObject e = new EventObject(this);
			// JOGLConfiguration.theLog.log(Level.INFO,"Viewer: broadcasting"+listeners.size()+" listeners");
			for (int i = 0; i < panelListeners.size(); ++i) {
				GLJPanelListener l = (GLJPanelListener) panelListeners.get(i);
				if (preRender)
					l.preRender(g2d);
				else
					l.postRender(g2d);
			}
		}
	}

	public GLJPanel getPanel() {
		return panel;
	}

	public void dispose(GLAutoDrawable drawable) {
		// TODO Auto-generated method stub

	}

	@Override
	public void installOverlay() {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void uninstallOverlay() {
		// TODO Auto-generated method stub
		
	}

}
