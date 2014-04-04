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

import java.awt.Component;
import java.awt.Dimension;
import java.awt.EventQueue;
import java.awt.event.KeyEvent;
import java.awt.event.KeyListener;
import java.awt.event.MouseEvent;
import java.awt.event.MouseListener;
import java.awt.event.MouseMotionListener;
import java.awt.event.MouseWheelEvent;
import java.awt.event.MouseWheelListener;
import java.awt.image.BufferedImage;
import java.lang.ref.WeakReference;
import java.lang.reflect.InvocationTargetException;
import java.util.EventObject;
import java.util.List;
import java.util.Vector;
import java.util.logging.Level;

import javax.media.opengl.DefaultGLCapabilitiesChooser;
import javax.media.opengl.GLAutoDrawable;
import javax.media.opengl.GLCapabilities;
import javax.media.opengl.GLContext;
import javax.media.opengl.GLEventListener;
import javax.swing.JPanel;

import de.jreality.math.Matrix;
import de.jreality.math.MatrixBuilder;
import de.jreality.scene.Camera;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SceneGraphPath;
import de.jreality.scene.StereoViewer;
import de.jreality.scene.Transformation;
import de.jreality.shader.Texture2D;
import de.jreality.util.CameraUtility;
import de.jreality.util.SceneGraphUtility;

abstract public class AbstractViewer implements de.jreality.scene.Viewer,
		InstrumentedViewer, StereoViewer, Runnable {
	protected SceneGraphComponent sceneRoot;
	SceneGraphComponent auxiliaryRoot;
	SceneGraphPath cameraPath;
	SceneGraphComponent cameraNode;
	protected GLAutoDrawable drawable;
	protected JOGLRenderer renderer;
	protected boolean disposed = false;
	int metric;
	boolean isFlipped = false;
	static WeakReference<GLContext> firstOne = new WeakReference<GLContext>(
			null); // for now, all display lists shared with this one
	public static final int CROSS_EYED_STEREO = 0;
	public static final int RED_BLUE_STEREO = 1;
	public static final int RED_GREEN_STEREO = 2;
	public static final int RED_CYAN_STEREO = 3;
	public static final int HARDWARE_BUFFER_STEREO = 4;
	public static final int LEFT_EYE_STEREO = 5; // <-- New
	public static final int RIGHT_EYE_STEREO = 6; // <-- New
	public static final int STEREO_TYPES = 7;
	protected int stereoType = CROSS_EYED_STEREO;
	protected boolean debug = false;

	public AbstractViewer() {
		this(null, null);
	}

	public AbstractViewer(SceneGraphPath camPath, SceneGraphComponent root) {
		setAuxiliaryRoot(SceneGraphUtility
				.createFullSceneGraphComponent("AuxiliaryRoot"));
		initializeFrom(root, camPath);
	}

	abstract void initializeFrom(SceneGraphComponent r, SceneGraphPath p);

	public GLAutoDrawable getDrawable() {
		return drawable;
	}

	public SceneGraphComponent getSceneRoot() {
		return sceneRoot;
	}

	public void setSceneRoot(SceneGraphComponent r) {
		if (r == null) {
			JOGLConfiguration.getLogger().log(Level.WARNING,
					"Null scene root, not setting.");
			return;
		}
		sceneRoot = r;
	}

	public SceneGraphComponent getAuxiliaryRoot() {
		return auxiliaryRoot;
	}

	public void setAuxiliaryRoot(SceneGraphComponent auxiliaryRoot) {
		this.auxiliaryRoot = auxiliaryRoot;
		if (renderer != null)
			renderer.setAuxiliaryRoot(auxiliaryRoot);
	}

	public SceneGraphPath getCameraPath() {
		return cameraPath;
	}

	public void setCameraPath(SceneGraphPath p) {
		cameraPath = p;
	}

	public void renderAsync() {
		if (disposed)
			return;
		synchronized (renderLock) {
			if (!pendingUpdate) {
				if (debug)
					JOGLConfiguration.theLog.log(Level.INFO,
							"Render: invoke later");
				EventQueue.invokeLater(this);
				pendingUpdate = true;
			}
		}
	}

	public boolean hasViewingComponent() {
		return true;
	}

	protected JPanel component;

	KeyListener keyListener = new KeyListener() {
		public void keyPressed(KeyEvent e) {
			component.dispatchEvent(e);
		}

		public void keyReleased(KeyEvent e) {
			component.dispatchEvent(e);
		}

		public void keyTyped(KeyEvent e) {
			component.dispatchEvent(e);
		}
	};
	MouseListener mouseListener = new MouseListener() {
		public void mouseClicked(MouseEvent e) {
			component.dispatchEvent(e);
		}

		public void mouseEntered(MouseEvent e) {
			component.dispatchEvent(e);
		}

		public void mouseExited(MouseEvent e) {
			component.dispatchEvent(e);
		}

		public void mousePressed(MouseEvent e) {
			component.dispatchEvent(e);
			((Component) drawable).requestFocus();
		}

		public void mouseReleased(MouseEvent e) {
			component.dispatchEvent(e);
		}
	};
	MouseWheelListener mouseWheelListener = new MouseWheelListener() {
		public void mouseWheelMoved(MouseWheelEvent e) {
			component.dispatchEvent(e);
		}
	};

	MouseMotionListener mouseMotionListener = new MouseMotionListener() {
		public void mouseDragged(MouseEvent e) {
			component.dispatchEvent(e);
		}

		public void mouseMoved(MouseEvent e) {
			component.dispatchEvent(e);
		}
	};

	public Object getViewingComponent() {
		// this is to avoid layout problems when returning the plain glcanvas
		if (component == null) {
			component = new javax.swing.JPanel();
			component.setLayout(new java.awt.BorderLayout());
			component.setMaximumSize(new java.awt.Dimension(32768, 32768));
			component.setMinimumSize(new java.awt.Dimension(10, 10));
			if (drawable == null)
				return component;
			component.add("Center", (Component) drawable);
			((Component) drawable).addKeyListener(keyListener);
			((Component) drawable).addMouseListener(mouseListener);
			((Component) drawable).addMouseMotionListener(mouseMotionListener);
			((Component) drawable).addMouseWheelListener(mouseWheelListener);
		}
		return component;
	}

	public Dimension getViewingComponentSize() {
		return ((Component) getViewingComponent()).getSize();
	}

	public void initializeFrom(de.jreality.scene.Viewer v) {
		initializeFrom(v.getSceneRoot(), v.getCameraPath());
	}

	/*********** Non-standard set/get ******************/

	public void setStereoType(int type) {
		if (renderer != null)
			renderer.setStereoType(type);
		stereoType = type;
	}

	// used in JOGLRenderer
	public int getStereoType() {
		return renderer.getStereoType();
	}

	public JOGLRenderer getRenderer() {
		return renderer;
	}

	
	@Override
	public void addGLEventListener(GLEventListener e) {
		drawable.addGLEventListener(e);
	}

	@Override
	public double getClockRate() {
		return renderer.getClockrate();
	}

	@Override
	public double getFrameRate() {
		return renderer.getFramerate();
	}

	@Override
	public int getPolygonCount() {
		return renderer.getPolygonCount();
	}

	/****** listeners! ************/
	Vector<RenderListener> listeners;

	public interface RenderListener extends java.util.EventListener {
		public void renderPerformed(EventObject e);
	}

	public void addRenderListener(AbstractViewer.RenderListener l) {
		if (listeners == null)
			listeners = new Vector<RenderListener>();
		if (listeners.contains(l))
			return;
		listeners.add(l);
		// JOGLConfiguration.theLog.log(Level.INFO,"Viewer: Adding geometry listener"+l+"to this:"+this);
	}

	public void removeRenderListener(AbstractViewer.RenderListener l) {
		if (listeners == null)
			return;
		listeners.remove(l);
	}

	public void broadcastChange() {
		if (listeners == null || renderingOffscreen)
			return;
		// SyJOGLConfiguration.theLog.log(Level.INFO,"Viewer: broadcasting"+listeners.size()+" listeners");
		if (!listeners.isEmpty()) {
			EventObject e = new EventObject(this);
			// JOGLConfiguration.theLog.log(Level.INFO,"Viewer: broadcasting"+listeners.size()+" listeners");
			for (int i = 0; i < listeners.size(); ++i) {
				AbstractViewer.RenderListener l = (AbstractViewer.RenderListener) listeners
						.get(i);
				l.renderPerformed(e);
			}
		}
	}

	/****** Convenience methods ************/
	public void addAuxiliaryComponent(SceneGraphComponent aux) {
		if (auxiliaryRoot == null) {
			setAuxiliaryRoot(SceneGraphUtility
					.createFullSceneGraphComponent("AuxiliaryRoot"));
		}
		if (!auxiliaryRoot.isDirectAncestor(aux))
			auxiliaryRoot.addChild(aux);
	}

	public void removeAuxiliaryComponent(SceneGraphComponent aux) {
		if (auxiliaryRoot == null)
			return;
		if (!auxiliaryRoot.isDirectAncestor(aux))
			return;
		auxiliaryRoot.removeChild(aux);
	}

	// Simple class to warn if results are not going to be as expected
	static class MultisampleChooser extends DefaultGLCapabilitiesChooser {
		public int chooseCapabilities(GLCapabilities desired,
				List<GLCapabilities> available,
				int windowSystemRecommendedChoice) {
			boolean anyHaveSampleBuffers = false;
			for (GLCapabilities caps : available) {
				// GLCapabilities caps = available[i];
				if (caps != null && caps.getSampleBuffers()) {
					anyHaveSampleBuffers = true;
					break;
				}
			}
			int selection = super.chooseCapabilities(desired, available,
					windowSystemRecommendedChoice);
			if (!anyHaveSampleBuffers) {
				JOGLConfiguration
						.getLogger()
						.log(Level.WARNING,
								"WARNING: antialiasing will be disabled because none of the available pixel formats had it to offer");
			} else {
				if (!available.get(selection).getSampleBuffers()) {
					JOGLConfiguration
							.getLogger()
							.log(Level.WARNING,
									"WARNING: antialiasing will be disabled because the DefaultGLCapabilitiesChooser didn't supply it");
				}
			}
			return selection;
		}
	}

	boolean renderingOffscreen = false;

	public BufferedImage renderOffscreen(int w, int h) {
		return renderOffscreen(null, w, h);
	}

	public BufferedImage renderOffscreen(BufferedImage dst, int w, int h) {
		return renderOffscreen(dst, w, h, 1.0);
	}

	public BufferedImage renderOffscreen(int w, int h, double aa) {
		return renderOffscreen(null, w, h, aa);
	}

	public BufferedImage renderOffscreen(BufferedImage dst, int w, int h,
			double aa) {
		if (renderer != null) {
			renderingOffscreen = true;
			renderer.offscreenRenderer.setAsTexture(false);
			dst = renderer.offscreenRenderer.renderOffscreen(dst, w, h, aa,
					drawable);
			renderingOffscreen = false;
			return dst;
		} else {
			JOGLConfiguration.getLogger().log(Level.WARNING,
					"Renderer not initialized");
			return null;
		}
	}

	public static Matrix[] cubeMapMatrices = new Matrix[6],
			textureMapMatrices = new Matrix[6];
	static {
		for (int i = 0; i < 6; i++) {
			cubeMapMatrices[i] = new Matrix();
			textureMapMatrices[i] = new Matrix();
		}
		MatrixBuilder.euclidean().rotateY(-Math.PI / 2)
				.assignTo(cubeMapMatrices[0]); // right
		MatrixBuilder.euclidean().rotateY(Math.PI / 2)
				.assignTo(cubeMapMatrices[1]); // left
		MatrixBuilder.euclidean().rotateX(Math.PI / 2)
				.assignTo(cubeMapMatrices[2]); // up
		MatrixBuilder.euclidean().rotateX(-Math.PI / 2)
				.assignTo(cubeMapMatrices[3]); // down
		MatrixBuilder.euclidean().rotateY(Math.PI).assignTo(cubeMapMatrices[5]); // back
																					// ...
																					// front
																					// (Id)
		// MatrixBuilder.euclidean().rotateZ(-Math.PI/2).assignTo(textureMapMatrices[2]);
		// // up
		// MatrixBuilder.euclidean().rotateZ(Math.PI/2).assignTo(textureMapMatrices[3]);
		// // down
	}

	public BufferedImage[] renderCubeMap(int size) {
		BufferedImage[] cmp = new BufferedImage[6];
		Camera cam = CameraUtility.getCamera(this);
		double oldFOV = cam.getFieldOfView();
		cam.setFieldOfView(90.0);
		SceneGraphComponent camNode = CameraUtility.getCameraNode(this);
		if (camNode.getTransformation() == null)
			camNode.setTransformation(new Transformation());
		Matrix oldCamMat = new Matrix(camNode.getTransformation().getMatrix());
		for (int i = 0; i < 6; ++i) {
			Matrix newCamMat = new Matrix(oldCamMat);
			newCamMat.multiplyOnRight(cubeMapMatrices[i]);
			newCamMat.assignTo(camNode);
			cmp[i] = renderOffscreen(cmp[i], size, size);
		}
		cam.setFieldOfView(oldFOV);
		camNode.getTransformation().setMatrix(oldCamMat.getArray());

		return cmp;

	}

	JOGLFBO[] fbos = new JOGLFBO[6];

	public void renderCubeMap(Texture2D[] texs, int size, boolean asTexture) {
		Camera cam = CameraUtility.getCamera(this);
		double oldFOV = cam.getFieldOfView();
		cam.setFieldOfView(90.0);
		SceneGraphComponent camNode = CameraUtility.getCameraNode(this);
		if (camNode.getTransformation() == null)
			camNode.setTransformation(new Transformation());
		Matrix oldCamMat = new Matrix(camNode.getTransformation().getMatrix());
		renderer.offscreenRenderer.setAsTexture(asTexture);
		for (int i = 0; i < 6; ++i) {
			Matrix newCamMat = new Matrix(oldCamMat);
			newCamMat.multiplyOnRight(cubeMapMatrices[i]);
			newCamMat.assignTo(camNode);
			renderingOffscreen = true;
			fbos[i] = renderer.offscreenRenderer.renderOffscreen(fbos[i],
					texs[i], size, size);
			renderingOffscreen = false;
		}
		cam.setFieldOfView(oldFOV);
		camNode.getTransformation().setMatrix(oldCamMat.getArray());

	}

	private boolean pendingUpdate;

	public void display(GLAutoDrawable arg0) {
		if (fbo != null)
			fbo.render(arg0.getGL().getGL2());
		renderer.display(arg0);
	}

	public void displayChanged(GLAutoDrawable arg0, boolean arg1, boolean arg2) {
		renderer.displayChanged(arg0, arg1, arg2);
	}

	public void init(GLAutoDrawable arg0) {
		JOGLConfiguration.theLog.log(Level.INFO,
				"JOGL Context initialization, creating new renderer");

		renderer = new JOGLRenderer(this);
		renderer.init(arg0);
		renderer.setStereoType(stereoType);
	}

	public void reshape(GLAutoDrawable arg0, int arg1, int arg2, int arg3,
			int arg4) {
		renderer.reshape(arg0, arg1, arg2, arg3, arg4);
	}

	protected final Object renderLock = new Object();
	boolean autoSwapBuffers = true;

	public boolean isRendering() {
		synchronized (renderLock) {
			return pendingUpdate;
		}
	}

	public void waitForRenderFinish() {
		synchronized (renderLock) {
			while (pendingUpdate)
				try {
					renderLock.wait();
				} catch (InterruptedException ex) {
				}
		}
	}

	public JOGLFBOViewer fbo;

	public void run() {
		if (!EventQueue.isDispatchThread())
			throw new IllegalStateException();
		synchronized (renderLock) {
			pendingUpdate = false;
			drawable.display();
			// JOGLConfiguration.theLog.log(Level.INFO,"rendering "+renderer.frameCount);
			if (listeners != null)
				broadcastChange();
			renderLock.notifyAll();
		}
		if (debug)
			JOGLConfiguration.theLog.log(Level.INFO, "Render: calling display");
	}

	public void setAutoSwapMode(boolean autoSwap) {
		autoSwapBuffers = autoSwap;
		drawable.setAutoSwapBufferMode(autoSwap);
	}

	final Runnable bufferSwapper = new Runnable() {
		public void run() {
			drawable.swapBuffers();
		}
	};

	public void swapBuffers() {
		if (EventQueue.isDispatchThread())
			drawable.swapBuffers();
		else
			try {
				EventQueue.invokeAndWait(bufferSwapper);
			} catch (InterruptedException e) {
				e.printStackTrace();
			} catch (InvocationTargetException e) {
				e.printStackTrace();
			}
	}

	public boolean canRenderAsync() {
		return true;
	}

	public void render() {
		if (disposed)
			return;
		if (EventQueue.isDispatchThread())
			run();
		else
			try {
				EventQueue.invokeAndWait(this);
			} catch (InterruptedException e) {
				e.printStackTrace();
			} catch (InvocationTargetException e) {
				e.printStackTrace();
			}
	}

	public void dispose() {
		disposed = true;
		//cameraPath.clear();
		//commenting this resolved the errors when switching to software viewer
		cameraNode = null;
		if (component != null) {
			((Component) drawable).removeKeyListener(keyListener);
			((Component) drawable).removeMouseListener(mouseListener);
			((Component) drawable)
					.removeMouseMotionListener(mouseMotionListener);
			((Component) drawable).removeMouseWheelListener(mouseWheelListener);
//			component.removeAll();
			//commenting this resolved the errors when switching to software viewer
			component = null;
		}
		setSceneRoot(null);
		setAuxiliaryRoot(null);
		if (listeners != null)
			listeners.clear();
		if (renderer != null)
			renderer.dispose();
		renderer = null;
	}
}
