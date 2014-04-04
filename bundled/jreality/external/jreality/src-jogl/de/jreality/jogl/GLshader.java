package de.jreality.jogl;

import java.io.BufferedReader;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.nio.ByteBuffer;
import java.nio.IntBuffer;

import javax.media.opengl.GL3;

public class GLshader {
	public int vertexShaderProgram;
	public int fragmentShaderProgram;
	public int shaderprogram;
	public String[] vsrc = null;
	public String[] fsrc = null;

	public void init(GL3 gl) {
		try {
			attachShaders(gl);
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	private String[] loadShaderSrc(String name) {
		StringBuilder sb = new StringBuilder();
		try {
			InputStream is = getClass().getResourceAsStream(name);
			BufferedReader br = new BufferedReader(new InputStreamReader(is));
			String line;
			while ((line = br.readLine()) != null) {
				sb.append(line);
				sb.append('\n');
			}
			is.close();
		} catch (Exception e) {
			e.printStackTrace();
		}
		System.out.println("Shader is " + sb.toString());
		return new String[] { sb.toString() };
	}

	public void loadVertexShaderSource(String name) {
		vsrc = loadShaderSrc(name);
	}

	public void loadFragmentShaderSource(String name) {
		fsrc = loadShaderSrc(name);
	}

	private void attachShaders(GL3 gl) throws Exception {
		shaderprogram = gl.glCreateProgram();
		if (vsrc != null) {
			vertexShaderProgram = gl.glCreateShader(GL3.GL_VERTEX_SHADER);
			gl.glShaderSource(vertexShaderProgram, 1, vsrc, null, 0);
			gl.glCompileShader(vertexShaderProgram);
			gl.glAttachShader(shaderprogram, vertexShaderProgram);
		}
		if (fsrc != null) {
			fragmentShaderProgram = gl.glCreateShader(GL3.GL_FRAGMENT_SHADER);
			gl.glShaderSource(fragmentShaderProgram, 1, fsrc, null, 0);
			gl.glCompileShader(fragmentShaderProgram);
			gl.glAttachShader(shaderprogram, fragmentShaderProgram);
		}

		gl.glLinkProgram(shaderprogram);
		gl.glValidateProgram(shaderprogram);
		IntBuffer intBuffer = IntBuffer.allocate(1);
		gl.glGetProgramiv(shaderprogram, GL3.GL_LINK_STATUS, intBuffer);
		if (intBuffer.get(0) != 1) {
			gl.glGetProgramiv(shaderprogram, GL3.GL_INFO_LOG_LENGTH, intBuffer);
			int size = intBuffer.get(0);
			System.err.println("Program link error: ");
			if (size > 0) {
				ByteBuffer byteBuffer = ByteBuffer.allocate(size);
				gl.glGetProgramInfoLog(shaderprogram, size, intBuffer,
						byteBuffer);
				for (byte b : byteBuffer.array()) {
					System.err.print((char) b);
				}
			} else {
				System.out.println("Unknown");
			}
			System.exit(1);
		}
	}

	public void useShader(GL3 gl) {
		gl.glUseProgram(shaderprogram);
	}

	public void dontUseShader(GL3 gl) {
		gl.glUseProgram(0);
	}
}