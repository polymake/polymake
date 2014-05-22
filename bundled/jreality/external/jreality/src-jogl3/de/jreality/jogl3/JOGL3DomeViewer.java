package de.jreality.jogl3;

import javax.media.opengl.GL3;
import javax.media.opengl.GLAutoDrawable;

import de.jreality.jogl3.glsl.GLShader;
import de.jreality.jogl3.shader.GLVBO;
import de.jreality.jogl3.shader.GLVBOFloat;
import de.jreality.math.MatrixBuilder;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SceneGraphPath;
import de.jreality.scene.Transformation;
import de.jreality.util.CameraUtility;

/**
 * @author Benjamin Kutschan
 */
public class JOGL3DomeViewer extends JOGL3Viewer {

	public JOGL3DomeViewer() throws Exception {
		super();
		// TODO Auto-generated constructor stub
	}

	private int[] texs = new int[6];
	private int[] fbos = new int[6];
	private int[] rbuffer = new int[6];

	private int width = 1024;
	private int height = 1024;
	private float FOV = 360;
	boolean zenith = false;

	private String[] name = new String[] { "texFront", "texRight", "texBack",
			"texLeft", "texTop", "texFloor" };

	private int textureUnit[] = new int[] { GL3.GL_TEXTURE0, GL3.GL_TEXTURE1,
			GL3.GL_TEXTURE2, GL3.GL_TEXTURE3, GL3.GL_TEXTURE4, GL3.GL_TEXTURE5 };

	private GLShader shader;
	private GLVBO vboVert;
	private GLVBO vboTex;

	@SuppressWarnings("static-access")
	public void init(GLAutoDrawable gla) {

		GL3 gl = gla.getGL().getGL3();

		// setup textures, renderbuffers and framebuffer objects
		gl.glGenTextures(6, texs, 0);
		gl.glGenRenderbuffers(6, rbuffer, 0);
		gl.glGenFramebuffers(6, fbos, 0);

		for (int i = 0; i < 6; i++) {
			// setup texture i
			gl.glBindTexture(gl.GL_TEXTURE_2D, texs[i]);
			gl.glTexParameteri(gl.GL_TEXTURE_2D, gl.GL_TEXTURE_MAG_FILTER,
					gl.GL_LINEAR);
			gl.glTexParameteri(gl.GL_TEXTURE_2D, gl.GL_TEXTURE_MIN_FILTER,
					gl.GL_LINEAR);

			gl.glTexParameteri(gl.GL_TEXTURE_2D, gl.GL_TEXTURE_WRAP_S,
					gl.GL_CLAMP_TO_EDGE);
			gl.glTexParameteri(gl.GL_TEXTURE_2D, gl.GL_TEXTURE_WRAP_T,
					gl.GL_CLAMP_TO_EDGE);

			// gl.glTexImage2D(gl.GL_TEXTURE_2D, 0, gl.GL_RGB, 512, 512, 0,
			// gl.GL_RGB, gl.GL_BYTE, null);
			gl.glTexImage2D(gl.GL_TEXTURE_2D, 0, gl.GL_RGBA8, width, height, 0,
					gl.GL_RGBA, gl.GL_UNSIGNED_BYTE, null);
			// setup renderbuffer i
			gl.glBindRenderbuffer(gl.GL_RENDERBUFFER, rbuffer[i]);
			gl.glRenderbufferStorage(gl.GL_RENDERBUFFER, gl.GL_DEPTH_COMPONENT,
					width, height);

			gl.glBindFramebuffer(gl.GL_FRAMEBUFFER, fbos[i]);
			// attach texture to framebuffer
			gl.glFramebufferTexture2D(gl.GL_FRAMEBUFFER,
					gl.GL_COLOR_ATTACHMENT0, gl.GL_TEXTURE_2D, texs[i], 0);
			// attach renderbuffer
			gl.glFramebufferRenderbuffer(gl.GL_FRAMEBUFFER,
					gl.GL_DEPTH_ATTACHMENT, gl.GL_RENDERBUFFER, rbuffer[i]);
		}

		gl.glBindFramebuffer(gl.GL_FRAMEBUFFER, 0);
		gl.glBindRenderbuffer(gl.GL_RENDERBUFFER, 0);

		// setup the shader program

		shader = new GLShader("dome.v", "dome.f");
		//shader.loadFragmentShaderSource("dome.f");
		//shader.loadVertexShaderSource("dome.v");
		shader.init(gl);

		// set up the vertex buffer object

		float[] vertdata = new float[] { -1, 1, 0, 1, 1, 0, 1, -1, 0, -1, 1, 0,
				1, -1, 0, -1, -1, 0 };
		float[] texdata = vertdata;

		vboVert = new GLVBOFloat(gl, vertdata, "in_Position");
		vboTex = new GLVBOFloat(gl, texdata, "tex_Coord");
		
		//vbo = new GLVBO(vertdata, texdata, shader.shaderprogram, "in_Position", "tex_Coord");
		//vbo.load(gl);

		// set up texture units

		// for(int i = 0; i < 6; i++){
		// gl.glActiveTexture(textureUnit[i]);
		// gl.glBindTexture(gl.GL_TEXTURE_2D, texs[i]);
		// gl.glUniform1i(gl.glGetUniformLocation(shader.shaderprogram,
		// name[i]), i);
		// }
		super.init(gla);

	}

	@SuppressWarnings("static-access")
	public void display(GLAutoDrawable gla) {

		GL3 gl = gla.getGL().getGL3();

		double[] axis = new double[3];
		double angle = 0;
		for (int i = 0; i < 6; i++) {
			// turn on FBO
			gl.glBindFramebuffer(gl.GL_FRAMEBUFFER, fbos[i]);

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

			CameraUtility.getCamera(this).setFieldOfView(90);

			SceneGraphPath sgp = getCameraPath();
			SceneGraphComponent sgc = (SceneGraphComponent) sgp.get(sgp
					.getLength() - 2);
			Transformation tr = sgc.getTransformation();

			if (zenith)
				MatrixBuilder.euclidean(tr)
						.rotate(Math.PI / 2, new double[] { 1, 0, 0 })
						.assignTo(tr);
			MatrixBuilder.euclidean(tr).rotate(angle, axis).assignTo(tr);

			//renderer.width = width;
			//renderer.height = height;
			super.display(gla);
			// super.display(gla, width, height);

			MatrixBuilder.euclidean(tr).rotate(-angle, axis).assignTo(tr);
			if (zenith)
				MatrixBuilder.euclidean(tr)
						.rotate(-Math.PI / 2, new double[] { 1, 0, 0 })
						.assignTo(tr);

		}

		// turn off FBO
		gl.glBindFramebuffer(gl.GL_FRAMEBUFFER, 0);

		// and render the cubemap

		// clear background
		gl.glClearColor(0f, 0f, 0f, 1f);
		gl.glClear(gl.GL_COLOR_BUFFER_BIT);

		// set viewport to window size

		// gl.glViewport(0, 0, renderer.theCanvas.getWidth(),
		// renderer.theCanvas.getHeight());
		gl.glViewport(0, 0,
				(int) Math.round(getViewingComponentSize().getWidth()),
				(int) Math.round(getViewingComponentSize().getHeight()));

		// disable depth test
		gl.glDisable(gl.GL_DEPTH_TEST);

		gl.glEnable(gl.GL_TEXTURE_2D);

		shader.useShader(gl);
		for (int i = 0; i < 6; i++) {
			gl.glActiveTexture(textureUnit[i]);
			gl.glBindTexture(gl.GL_TEXTURE_2D, texs[i]);
			gl.glUniform1i(
					gl.glGetUniformLocation(shader.shaderprogram, name[i]), i);
		}
		gl.glUniform1f(gl.glGetUniformLocation(shader.shaderprogram, "angle"),
				FOV);

		//vboVert.display(gl);
		gl.glBindBuffer(gl.GL_ARRAY_BUFFER, vboVert.getID());
		gl.glVertexAttribPointer(gl.glGetAttribLocation(shader.shaderprogram, vboVert.getName()), 3,
				gl.GL_FLOAT, false, 0, 0);
		gl.glEnableVertexAttribArray(gl.glGetAttribLocation(shader.shaderprogram, vboVert.getName()));

		//if (texdata != null) {
			gl.glBindBuffer(gl.GL_ARRAY_BUFFER, vboTex.getID());
			gl.glVertexAttribPointer(gl.glGetAttribLocation(shader.shaderprogram, vboTex.getName()),
					3, gl.GL_FLOAT, false, 0, 0);
			gl.glEnableVertexAttribArray(gl
					.glGetAttribLocation(shader.shaderprogram, vboTex.getName()));
		//}

		gl.glDrawArrays(gl.GL_TRIANGLES, 0, vboVert.getLength() / 3);
		gl.glDisableVertexAttribArray(gl.glGetAttribLocation(shader.shaderprogram, vboVert.getName()));
		//if (texdata != null)
			gl.glDisableVertexAttribArray(gl.glGetAttribLocation(shader.shaderprogram, vboTex.getName()));
		
		
		shader.dontUseShader(gl);

		// "restore" state
		gl.glActiveTexture(textureUnit[0]);
		gl.glBindTexture(gl.GL_TEXTURE_2D, 0);
		gl.glEnable(gl.GL_DEPTH_TEST);

		gl.glDisable(gl.GL_TEXTURE);

	}
}