package de.jreality.jogl;

import java.nio.FloatBuffer;

import javax.media.opengl.GL3;

public class GLvbo {

	private float[] vertdata = null;
	private float[] texdata = null;
	private int[] vertindex = new int[1];
	private int[] texindex = new int[1];
	private int shader = 0;
	private String vertname = null;
	private String texname = null;

	GLvbo(float[] vertdata, float[] texdata, int shader, String vertname,
			String texname) {
		if (vertdata == null)
			System.out.println("Error: no vertex data specified");
		this.vertdata = vertdata;
		this.texdata = texdata;
		this.vertname = vertname;
		this.texname = texname;
		this.shader = shader;
	}

	public void load(GL3 gl) {
		gl.glGenBuffers(1, vertindex, 0);
		gl.glBindBuffer(gl.GL_ARRAY_BUFFER, vertindex[0]);
		gl.glBufferData(gl.GL_ARRAY_BUFFER, 4 * vertdata.length,
				FloatBuffer.wrap(vertdata), gl.GL_STATIC_DRAW);

		if (texdata != null) {
			gl.glGenBuffers(1, texindex, 0);
			gl.glBindBuffer(gl.GL_ARRAY_BUFFER, texindex[0]);
			gl.glBufferData(gl.GL_ARRAY_BUFFER, 4 * texdata.length,
					FloatBuffer.wrap(texdata), gl.GL_STATIC_DRAW);
		}
	}

	public void displayPoints(GL3 gl) {
		gl.glBindBuffer(gl.GL_ARRAY_BUFFER, vertindex[0]);
		gl.glVertexAttribPointer(gl.glGetAttribLocation(shader, vertname), 3,
				gl.GL_FLOAT, false, 0, 0);
		gl.glEnableVertexAttribArray(gl.glGetAttribLocation(shader, vertname));

		// if(texdata != null){
		// gl.glBindBuffer(gl.GL_ARRAY_BUFFER, texindex[0]);
		// gl.glVertexAttribPointer(gl.glGetAttribLocation(shader, texname), 3,
		// gl.GL_FLOAT, false, 0, 0);
		// gl.glEnableVertexAttribArray(gl.glGetAttribLocation(shader,
		// texname));
		// }

		gl.glDrawArrays(gl.GL_POINTS, 0, vertdata.length);
		gl.glDisableVertexAttribArray(gl.glGetAttribLocation(shader, vertname));
		// if(texdata != null)
		// gl.glDisableVertexAttribArray(gl.glGetAttribLocation(shader,
		// texname));
	}

	public void display(GL3 gl) {
		gl.glBindBuffer(gl.GL_ARRAY_BUFFER, vertindex[0]);
		gl.glVertexAttribPointer(gl.glGetAttribLocation(shader, vertname), 3,
				gl.GL_FLOAT, false, 0, 0);
		gl.glEnableVertexAttribArray(gl.glGetAttribLocation(shader, vertname));

		if (texdata != null) {
			gl.glBindBuffer(gl.GL_ARRAY_BUFFER, texindex[0]);
			gl.glVertexAttribPointer(gl.glGetAttribLocation(shader, texname),
					3, gl.GL_FLOAT, false, 0, 0);
			gl.glEnableVertexAttribArray(gl
					.glGetAttribLocation(shader, texname));
		}

		gl.glDrawArrays(gl.GL_TRIANGLES, 0, vertdata.length / 3);
		gl.glDisableVertexAttribArray(gl.glGetAttribLocation(shader, vertname));
		if (texdata != null)
			gl.glDisableVertexAttribArray(gl.glGetAttribLocation(shader,
					texname));
	}
}
