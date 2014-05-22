package de.jreality.jogl;

import javax.media.opengl.GL;
import javax.media.opengl.GL2;
import javax.media.opengl.GLAutoDrawable;

import de.jreality.math.MatrixBuilder;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SceneGraphPath;
import de.jreality.scene.Transformation;
import de.jreality.util.CameraUtility;

//DEPRECATED!!!
public class DomeViewer2 extends Viewer {

	private int[] texs = new int[6];
	private int[] fbos = new int[6];
	private int[] rbuffer = new int[6];

	private int width = 1024;
	private int height = 1024;

	public void init(GLAutoDrawable gla) {

		GL2 gl = gla.getGL().getGL2();

		// setup textures, renderbuffers and framebuffer objects
		gl.glGenTextures(6, texs, 0);
		gl.glGenRenderbuffers(6, rbuffer, 0);
		gl.glGenFramebuffers(6, fbos, 0);

		for (int i = 0; i < 6; i++) {
			// setup texture i
			gl.glBindTexture(GL.GL_TEXTURE_2D, texs[i]);
			gl.glTexParameteri(GL.GL_TEXTURE_2D, GL.GL_TEXTURE_MAG_FILTER,
					GL.GL_LINEAR);
			gl.glTexParameteri(GL.GL_TEXTURE_2D, GL.GL_TEXTURE_MIN_FILTER,
					GL.GL_LINEAR);

			gl.glTexParameteri(GL.GL_TEXTURE_2D, GL.GL_TEXTURE_WRAP_S,
					GL.GL_CLAMP_TO_EDGE);
			gl.glTexParameteri(GL.GL_TEXTURE_2D, GL.GL_TEXTURE_WRAP_T,
					GL.GL_CLAMP_TO_EDGE);

			// gl.glTexImage2D(gl.GL_TEXTURE_2D, 0, gl.GL_RGB, 512, 512, 0,
			// gl.GL_RGB, gl.GL_BYTE, null);
			gl.glTexImage2D(GL.GL_TEXTURE_2D, 0, GL.GL_RGBA8, width, height, 0,
					GL.GL_RGBA, GL.GL_UNSIGNED_BYTE, null);
			// setup renderbuffer i
			gl.glBindRenderbuffer(GL.GL_RENDERBUFFER, rbuffer[i]);
			gl.glRenderbufferStorage(GL.GL_RENDERBUFFER,
					GL2.GL_DEPTH_COMPONENT, width, height);

			gl.glBindFramebuffer(GL.GL_FRAMEBUFFER, fbos[i]);
			// attach texture to framebuffer
			gl.glFramebufferTexture2D(GL.GL_FRAMEBUFFER,
					GL.GL_COLOR_ATTACHMENT0, GL.GL_TEXTURE_2D, texs[i], 0);
			// attach renderbuffer
			gl.glFramebufferRenderbuffer(GL.GL_FRAMEBUFFER,
					GL.GL_DEPTH_ATTACHMENT, GL.GL_RENDERBUFFER, rbuffer[i]);
		}

		gl.glBindFramebuffer(GL.GL_FRAMEBUFFER, 0);
		gl.glBindRenderbuffer(GL.GL_RENDERBUFFER, 0);

		// g_textureLocation = glGetUniformLocation(g_program.program,
		// "u_texture");

		super.init(gla);

	}

	public void display(GLAutoDrawable gla) {

		// GL3 gl = gla.getGL().getGL3();
		GL2 gl2 = gla.getGL().getGL2();

		double[] axis = new double[3];
		double angle = 0;
		for (int i = 0; i < 6; i++) {
			// turn on FBO
			gl2.glBindFramebuffer(GL.GL_FRAMEBUFFER, fbos[i]);

			switch (i) {
			case 0:
				axis = new double[] { 0, 1, 0 };
				angle = 0;
				break;
			case 1:
				axis = new double[] { 0, 1, 0 };
				angle = -Math.PI / 2;
				break;
			case 2:
				axis = new double[] { 0, 1, 0 };
				angle = Math.PI;
				break;
			case 3:
				axis = new double[] { 0, 1, 0 };
				angle = Math.PI / 2;
				break;
			case 4:
				axis = new double[] { 1, 0, 0 };
				angle = Math.PI / 2;
				break;
			case 5:
				axis = new double[] { 1, 0, 0 };
				angle = -Math.PI / 2;
				break;
			}

			CameraUtility.getCamera(renderer.theViewer).setFieldOfView(90);

			SceneGraphPath sgp = renderer.theViewer.getCameraPath();
			SceneGraphComponent sgc = (SceneGraphComponent) sgp.get(sgp
					.getLength() - 2);
			Transformation tr = sgc.getTransformation();
			MatrixBuilder.euclidean(tr).rotate(angle, axis).assignTo(tr);

			renderer.width = width;
			renderer.height = height;
			super.display(gla);

			MatrixBuilder.euclidean(tr).rotate(-angle, axis).assignTo(tr);
		}

		// turn off FBO
		gl2.glBindFramebuffer(GL.GL_FRAMEBUFFER, 0);

		// and render the cubemap

		// clear background
		gl2.glClearColor(0f, 0f, 0f, 1f);
		gl2.glClear(gl2.GL_COLOR_BUFFER_BIT);

		// set viewport to window size
		gl2.glViewport(0, 0, renderer.theCanvas.getWidth(),
				renderer.theCanvas.getHeight());

		// clear transformation matrices
		gl2.glMatrixMode(gl2.GL_MODELVIEW);
		gl2.glLoadIdentity();
		gl2.glMatrixMode(gl2.GL_PROJECTION);
		gl2.glLoadIdentity();
		gl2.glMatrixMode(gl2.GL_TEXTURE);
		gl2.glLoadIdentity();

		// disable depth test and lighting
		gl2.glDisable(GL.GL_DEPTH_TEST);
		gl2.glDisable(gl2.GL_LIGHTING);

		gl2.glEnable(gl2.GL_TEXTURE_2D);
		gl2.glColor3f(1, 1f, 1f);
		// funny texture corrdinats neccessary. why?
		int texcoord = 1;
		gl2.glHint(gl2.GL_PERSPECTIVE_CORRECTION_HINT, gl2.GL_NICEST);
		// front (0)
		gl2.glBindTexture(GL.GL_TEXTURE_2D, texs[0]);
		gl2.glBegin(gl2.GL_QUADS);

		gl2.glTexCoord2i(0, texcoord);
		gl2.glVertex2d(-.5, .5);

		gl2.glTexCoord2i(texcoord, texcoord);
		gl2.glVertex2d(.5, .5);

		gl2.glTexCoord2i(texcoord, 0);
		gl2.glVertex2d(.5, 0);

		gl2.glTexCoord2i(0, 0);
		gl2.glVertex2d(-.5, 0);

		gl2.glEnd();

		// right (1)
		gl2.glBindTexture(GL.GL_TEXTURE_2D, texs[1]);
		gl2.glBegin(gl2.GL_QUADS);

		gl2.glTexCoord2f(0, texcoord);
		gl2.glVertex2d(.5, .5);

		gl2.glTexCoord2f(texcoord, texcoord);
		gl2.glVertex2d(1, 1);

		gl2.glTexCoord2f(texcoord, 0);
		gl2.glVertex2d(1, -.5);

		gl2.glTexCoord2f(0, 0);
		gl2.glVertex2d(.5, 0);

		gl2.glEnd();

		// left (3)
		gl2.glBindTexture(GL.GL_TEXTURE_2D, texs[3]);
		gl2.glBegin(gl2.GL_QUADS);

		gl2.glTexCoord2f(0, texcoord);
		gl2.glVertex2d(-1, .5);

		gl2.glTexCoord2f(texcoord, texcoord);
		gl2.glVertex2d(-.5, .5);

		gl2.glTexCoord2f(texcoord, 0);
		gl2.glVertex2d(-.5, 0);

		gl2.glTexCoord2f(0, 0);
		gl2.glVertex2d(-1, 0);

		gl2.glEnd();

		// back (2)
		gl2.glBindTexture(GL.GL_TEXTURE_2D, texs[2]);
		gl2.glBegin(gl2.GL_QUADS);

		gl2.glTexCoord2f(0, texcoord);
		gl2.glVertex2d(.5, -1);

		gl2.glTexCoord2f(texcoord, texcoord);
		gl2.glVertex2d(-.5, -1);

		gl2.glTexCoord2f(texcoord, 0);
		gl2.glVertex2d(-.5, -.5);

		gl2.glTexCoord2f(0, 0);
		gl2.glVertex2d(.5, -.5);

		gl2.glEnd();

		// up (4)
		gl2.glBindTexture(GL.GL_TEXTURE_2D, texs[4]);
		gl2.glBegin(gl2.GL_QUADS);

		gl2.glTexCoord2f(0, texcoord);
		gl2.glVertex2d(-.5, 1);

		gl2.glTexCoord2f(texcoord, texcoord);
		gl2.glVertex2d(.5, 1);

		gl2.glTexCoord2f(texcoord, 0);
		gl2.glVertex2d(.5, .5);

		gl2.glTexCoord2f(0, 0);
		gl2.glVertex2d(-.5, .5);

		gl2.glEnd();

		// down (5)
		gl2.glBindTexture(GL.GL_TEXTURE_2D, texs[5]);
		gl2.glBegin(gl2.GL_QUADS);

		gl2.glTexCoord2f(0, texcoord);
		gl2.glVertex2d(-.5, 0);

		gl2.glTexCoord2f(texcoord, texcoord);
		gl2.glVertex2d(.5, 0);

		gl2.glTexCoord2f(texcoord, 0);
		gl2.glVertex2d(.5, -.5);

		gl2.glTexCoord2f(0, 0);
		gl2.glVertex2d(-.5, -.5);

		gl2.glEnd();

		// "restore" state
		gl2.glBindTexture(GL.GL_TEXTURE_2D, 0);
		gl2.glEnable(GL.GL_DEPTH_TEST);
		gl2.glEnable(gl2.GL_LIGHTING);
		gl2.glDisable(gl2.GL_TEXTURE);

	}
}