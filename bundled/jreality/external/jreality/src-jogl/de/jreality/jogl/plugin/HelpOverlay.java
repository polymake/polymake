/*
 * Created on Jun 18, 2004
 *
 */
package de.jreality.jogl.plugin;

import java.awt.Dimension;
import java.awt.event.KeyEvent;
import java.util.ArrayList;
import java.util.List;

import javax.media.opengl.GL;
import javax.media.opengl.GL2;
import javax.media.opengl.GLAutoDrawable;
import javax.media.opengl.GLEventListener;
import javax.media.opengl.glu.GLU;
import javax.swing.KeyStroke;

import com.jogamp.opengl.util.gl2.GLUT;

import de.jreality.jogl.JOGLViewer;

/**
 * @author Pepijn Van Eeckhoudt
 */
public class HelpOverlay implements GLEventListener {
	JOGLViewer viewer;

	/**
	 * @param v
	 */
	public HelpOverlay(JOGLViewer v) {
		viewer = v;
	}

	private List keyboardEntries = new ArrayList();
	private List mouseEntries = new ArrayList();
	private boolean visible = false;
	private GLUT glut = new GLUT();
	private static final int CHAR_HEIGHT = 12;
	private static final int OFFSET = 15;
	private static final int INDENT = 3;
	private static final String KEYBOARD_CONTROLS = "Keyboard controls";
	private static final String MOUSE_CONTROLS = "Mouse controls";

	public boolean isVisible() {
		return visible;
	}

	public void setVisible(boolean visible) {
		this.visible = visible;
		if (visible) {
			mouseEntries.clear();
			// ToolManager.toolManagerForViewer(viewer).getCurrentTool().registerHelp(this);
		}
	}

	public void display(GLAutoDrawable glDrawable) {
		if (!visible)
			return;
		GL2 gl = glDrawable.getGL().getGL2();
		GLU glu = new GLU();

		// Store old matrices
		gl.glMatrixMode(GL2.GL_MODELVIEW);
		gl.glPushMatrix();
		gl.glLoadIdentity();
		gl.glMatrixMode(GL2.GL_PROJECTION);
		gl.glPushMatrix();
		gl.glLoadIdentity();

		Dimension size = new Dimension(glDrawable.getWidth(),
				glDrawable.getHeight());
		gl.glViewport(0, 0, size.width, size.height);

		// Store enabled state and disable lighting, texture mapping and the
		// depth buffer
		gl.glPushAttrib(GL2.GL_ENABLE_BIT);
		gl.glDisable(GL.GL_BLEND);
		gl.glDisable(GL2.GL_LIGHTING);
		gl.glDisable(GL.GL_TEXTURE_2D);
		gl.glDisable(GL.GL_DEPTH_TEST);
		for (int i = 0; i < 6; ++i)
			gl.glDisable(i + GL2.GL_CLIP_PLANE0);

		// Retrieve the current viewport and switch to orthographic mode
		int viewPort[] = new int[4];
		gl.glGetIntegerv(GL.GL_VIEWPORT, viewPort, 0);
		glu.gluOrtho2D(0, viewPort[2], viewPort[3], 0);

		// Render the text
		gl.glColor3f(1, 1, 1);

		int x = OFFSET;
		int maxx = 0;
		int y = OFFSET + CHAR_HEIGHT;

		if (keyboardEntries.size() > 0) {
			gl.glRasterPos2i(x, y);
			glut.glutBitmapString(GLUT.BITMAP_HELVETICA_12, KEYBOARD_CONTROLS);
			maxx = Math.max(
					maxx,
					OFFSET
							+ glut.glutBitmapLength(GLUT.BITMAP_HELVETICA_12,
									KEYBOARD_CONTROLS));

			y += OFFSET;
			x += INDENT;
			for (int i = 0; i < keyboardEntries.size(); i++) {
				gl.glRasterPos2f(x, y);
				String text = (String) keyboardEntries.get(i);
				glut.glutBitmapString(GLUT.BITMAP_HELVETICA_12, text);
				maxx = Math.max(
						maxx,
						OFFSET
								+ glut.glutBitmapLength(
										GLUT.BITMAP_HELVETICA_12, text));
				y += OFFSET;
			}
		}

		if (mouseEntries.size() > 0) {
			x = maxx + OFFSET;
			y = OFFSET + CHAR_HEIGHT;
			gl.glRasterPos2i(x, y);
			glut.glutBitmapString(GLUT.BITMAP_HELVETICA_12, MOUSE_CONTROLS);

			y += OFFSET;
			x += INDENT;
			for (int i = 0; i < mouseEntries.size(); i++) {
				gl.glRasterPos2f(x, y);
				glut.glutBitmapString(GLUT.BITMAP_HELVETICA_12,
						(String) mouseEntries.get(i));
				y += OFFSET;
			}
		}

		gl.glPopAttrib();

		// Restore old matrices
		gl.glPopMatrix();
		gl.glMatrixMode(GL2.GL_MODELVIEW);
		gl.glPopMatrix();
	}

	public void displayChanged(GLAutoDrawable glDrawable, boolean b, boolean b1) {
		// TODO document this
	}

	public void init(GLAutoDrawable glDrawable) {
		// TODO document this
	}

	public void reshape(GLAutoDrawable glDrawable, int i, int i1, int i2, int i3) {
		// TODO document this
	}

	public void registerKeyStroke(KeyStroke keyStroke, String description) {
		String modifiersText = KeyEvent.getKeyModifiersText(keyStroke
				.getModifiers());
		String keyText = KeyEvent.getKeyText(keyStroke.getKeyCode());
		keyboardEntries.add((modifiersText.length() != 0 ? modifiersText + " "
				: "") + keyText + ": " + description);
	}

	public void registerInfoString(String eventDesc, String description) {
		mouseEntries.add(eventDesc + ": " + description);

	}

	public void printOut() {
		for (int i = 0; i < keyboardEntries.size(); i++) {
			String text = (String) keyboardEntries.get(i);
			System.out.println(text);
		}

	}

	public void dispose(GLAutoDrawable drawable) {
		// TODO Auto-generated method stub

	}
}
