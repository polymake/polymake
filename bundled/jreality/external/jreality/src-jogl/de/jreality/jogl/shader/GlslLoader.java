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

package de.jreality.jogl.shader;

import java.beans.Statement;
import java.nio.ByteBuffer;
import java.nio.IntBuffer;
import java.util.HashMap;
import java.util.WeakHashMap;

import javax.media.opengl.GL;
import javax.media.opengl.GL2;

import de.jreality.jogl.JOGLRenderer;
import de.jreality.shader.GlslProgram;
import de.jreality.shader.GlslSource;
import de.jreality.shader.GlslSource.AttributeParameter;
import de.jreality.shader.GlslSource.UniformParameter;

public class GlslLoader {

	private static final WeakHashMap GL_TO_GLSL = new WeakHashMap();

	public static void render(GlslProgram prog, JOGLRenderer jr) {
		GL2 gl = jr.globalGL;
		render(prog, gl);
	}

	public static void render(GlslProgram prog, GL2 gl) {
		// System.err.println("in glslLoader render()");
		ProgramContext context = getContext(gl, prog);
		context.linkProgram(gl);
		context.activateProgram(gl);
		// now set all (changed) values
		for (UniformParameter param : prog.getSource().getUniformParameters()) {
			Object value = prog.getUniform(param.getName());
			Object oldValue = context.getUniform(param);
			// System.out.println("checking "+param);
			// System.out.println("\t old="+oldValue+ " new="+value);
			if (compare(oldValue, value, param.getPrimitiveType())) {
				context.writeValue(gl, param, value);
				// System.out.println("putting "+param);
			}
		}
		int k = 9;
		for (AttributeParameter param : prog.getSource().getAttributes()) {
			gl.glBindAttribLocation(context.progID.intValue(), k,
					param.getName());
			// System.err.println("Binding parameter "+param.getName()+"to "+k);
			k++;
		}
	}

	private static boolean compare(Object oldValue, Object value,
			Class primitive) {
		if (value == null)
			return false;
		if (oldValue == null)
			return true;
		if (primitive == float.class) {
			float[] old = (float[]) oldValue;
			float[] newF = (float[]) value;
			if (old.length != newF.length)
				return true;
			for (int i = 0; i < old.length; i++)
				if (old[i] != newF[i])
					return true;
		} else {
			int[] old = (int[]) oldValue;
			int[] newF = (int[]) value;
			if (old.length != newF.length)
				return true;
			for (int i = 0; i < old.length; i++)
				if (old[i] != newF[i])
					return true;
		}
		return false;
	}

	public static void postRender(GlslProgram prog, JOGLRenderer jr) {
		GL2 gl = jr.globalGL;
		postRender(prog, gl);
	}

	public static void postRender(GlslProgram prog, GL2 gl) {
		ProgramContext context = getContext(gl, prog);
		context.deactivateProgram(gl);
	}

	private static ProgramContext getContext(GL gl, GlslProgram prog) {
		WeakHashMap glContexts = (WeakHashMap) GL_TO_GLSL.get(gl);
		if (glContexts == null) {
			glContexts = new WeakHashMap();
			GL_TO_GLSL.put(gl, glContexts);
		}
		ProgramContext context = (ProgramContext) glContexts.get(prog
				.getSource());
		if (context == null) {
			context = new ProgramContext(prog.getSource());
			glContexts.put(prog.getSource(), context);
		}
		return context;
	}

	private static class ProgramContext {
		final GlslSource source;
		final HashMap currentValues = new HashMap();
		boolean isLinked;
		Integer progID;

		ProgramContext(GlslSource source) {
			this.source = source;
		}

		public void activateProgram(GL2 gl) {
			gl.glUseProgramObjectARB(progID.intValue());
		}

		public void deactivateProgram(GL2 gl) {
			gl.glUseProgramObjectARB(0);
		}

		Object getUniform(UniformParameter parameter) {
			return currentValues.get(parameter);
		}

		void writeValue(GL2 gl, UniformParameter param, Object value) {
			String rep = param.getStringRep();
			Object[] params = new Object[param.isMatrix() ? 5 : 4];
			params[0] = uniLocation(param.getName(), gl);
			if (((Integer) params[0]).intValue() == -1) {
				return;
			}
			params[1] = new Integer(param.isArray() ? param.getArrayLength()
					: 1);
			if (param.isMatrix())
				params[2] = Boolean.FALSE;
			params[param.isMatrix() ? 3 : 2] = value;
			params[param.isMatrix() ? 4 : 3] = new Integer(0);
			Statement s = new Statement(gl, rep, params);
			// System.out.println("will call: "+s);
			try {
				s.execute();
				// printInfoLog(param.toString(), progID.intValue(), gl);
			} catch (Exception e) {
				printInfoLog(param.toString(), progID.intValue(), gl);
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
			currentValues.put(param, value);
		}

		void linkProgram(GL2 gl) {
			if (isLinked)
				return;

			progID = new Integer(gl.glCreateProgramObjectARB());

			if (source.getVertexProgram() != null) {
				int vertexProgID = gl
						.glCreateShaderObjectARB(GL2.GL_VERTEX_SHADER);
				gl.glShaderSource(vertexProgID,
						source.getVertexProgram().length,
						source.getVertexProgram(), (int[]) null, 0);
				gl.glCompileShaderARB(vertexProgID);
				printInfoLog("vert compile", vertexProgID, gl);
				gl.glAttachObjectARB(progID.intValue(), vertexProgID);
				printInfoLog("vert attatch", vertexProgID, gl);
			}
			if (source.getFragmentProgram() != null) {
				int fragmentProgID = gl
						.glCreateShaderObjectARB(GL2.GL_FRAGMENT_SHADER);
				gl.glShaderSourceARB(fragmentProgID,
						source.getFragmentProgram().length,
						source.getFragmentProgram(), (int[]) null, 0);
				gl.glCompileShaderARB(fragmentProgID);
				printInfoLog("frag compile", fragmentProgID, gl);
				gl.glAttachObjectARB(progID.intValue(), fragmentProgID);
				printInfoLog("frag attatch", fragmentProgID, gl);
			}

			printInfoLog("prog attatch", progID.intValue(), gl);
			gl.glLinkProgramARB(progID.intValue());
			printInfoLog("prog link", progID.intValue(), gl);
			// System.out.println("loaded program ["+progID+"]");
			isLinked = true;
		}

		private Integer uniLocation(String name, GL2 gl) {
			int loc;
			loc = gl.glGetUniformLocationARB(progID.intValue(), name);
			if (loc == -1) {
				// this can happen easily since parameters that aren't used are
				// optimized away
				// don't want to throw an exception in this case.
				// throw new IllegalStateException("failed uniLoc for "+name);
			}
			return new Integer(loc);
		}
	}

	public static void printInfoLog(String name, int objectHandle, GL2 gl) {
		int[] logLength = new int[1];
		int[] charsWritten = new int[1];
		byte[] infoLog;

		gl.glGetObjectParameterivARB(objectHandle,
				GL2.GL_OBJECT_INFO_LOG_LENGTH_ARB, IntBuffer.wrap(logLength));

		if (logLength[0] > 0) {
			infoLog = new byte[logLength[0]];
			gl.glGetInfoLogARB(objectHandle, logLength[0],
					IntBuffer.wrap(charsWritten), ByteBuffer.wrap(infoLog));
			StringBuffer foo = new StringBuffer(charsWritten[0]);

			for (int i = 0; i < charsWritten[0]; ++i)
				foo.append((char) infoLog[i]);
			 if (foo.length() > 0)
			 System.out.println("["+name+"] GLSL info log: "+foo.toString());
		}
	}

	public static void dispose(GL2 gl, GlslProgram prog) {
		ProgramContext context = getContext(gl, prog);
		if (context == null) {
			System.out.println("Context NULL while disposing!!!");
			return;
		}
		Integer id = context.progID;
		if (id == null) {
			System.out.println("id NULL while disposing!!!");
			return;
		}
		gl.glDeleteProgramsARB(1, new int[] { id.intValue() }, 0);
		WeakHashMap glContexts = (WeakHashMap) GL_TO_GLSL.get(gl);
		glContexts.remove(prog.getSource());
	}

}
