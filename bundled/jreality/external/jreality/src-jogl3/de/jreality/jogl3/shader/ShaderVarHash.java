package de.jreality.jogl3.shader;

import java.util.WeakHashMap;

import javax.media.opengl.GL3;

import de.jreality.jogl3.glsl.GLShader;

/**
 * 
 * @author benjamin
 * 
 * This class is for storing uniform variable locations. Every uniform variable in a glsl shader
 * file has an integer describing its location in memory. This location can be queried with a GL call, which is however
 * rather expensive. Therefore this class stores for every shader the uniform locations. When binding the value of a
 * uniform this location is needed.
 */
public class ShaderVarHash {
	private static WeakHashMap<GLShader, WeakHashMap<String, Integer>> variables = new WeakHashMap<GLShader, WeakHashMap<String, Integer>>();
	
	public static void bindUniform(GLShader s, String name, int value, GL3 gl){
		WeakHashMap<String, Integer> shaders;
		shaders = variables.get(s);
		
		if(shaders == null){
			//System.out.println("Int: creating new shader hash map");
			shaders = new WeakHashMap<String, Integer>();
			variables.put(s, shaders);
		}
		Integer i = shaders.get(name);
		if(i == null){
			//System.out.println("Int: creating new shader-uniform pair in hash map");
			i = gl.glGetUniformLocation(s.shaderprogram, name);
			shaders.put(name, i);
		}
		gl.glUniform1i(i, value);
	}
	
	public static void bindUniform(GLShader s, String name, float value, GL3 gl){
		WeakHashMap<String, Integer> shaders;
		shaders = variables.get(s);
		
		if(shaders == null){
			//System.out.println("Float: creating new shader hash map");
			shaders = new WeakHashMap<String, Integer>();
			variables.put(s, shaders);
		}
		Integer i = shaders.get(name);
		if(i == null){
			//System.out.println("Float: creating new shader-uniform pair in hash map");
			i = gl.glGetUniformLocation(s.shaderprogram, name);
			shaders.put(name, i);
		}
		gl.glUniform1f(i, value);
	}
	
	public static void bindUniform(GLShader s, String name, float[] value, GL3 gl){
		WeakHashMap<String, Integer> shaders;
		shaders = variables.get(s);
		
		if(shaders == null){
			//System.out.println("Float[]: creating new shader hash map");
			shaders = new WeakHashMap<String, Integer>();
			variables.put(s, shaders);
		}
		Integer i = shaders.get(name);
		if(i == null){
			//System.out.println("Float[]: creating new shader-uniform pair in hash map");
			i = gl.glGetUniformLocation(s.shaderprogram, name);
			shaders.put(name, i);
		}
		if(value.length == 4)
			gl.glUniform4fv(i, 1, value, 0);
		if(value.length == 3)
			gl.glUniform3fv(i, 1, value, 0);
		if(value.length == 2)
			gl.glUniform2fv(i, 1, value, 0);
	}
	
	public static void bindUniformMatrix(GLShader s, String name, float[] value, GL3 gl){
		WeakHashMap<String, Integer> shaders;
		shaders = variables.get(s);
		
		if(shaders == null){
			//System.out.println("Matrix: creating new shader hash map");
			shaders = new WeakHashMap<String, Integer>();
			variables.put(s, shaders);
		}
		Integer i = shaders.get(name);
		if(i == null){
			//System.out.println("Matrix: creating new shader-uniform pair in hash map");
			i = gl.glGetUniformLocation(s.shaderprogram, name);
			shaders.put(name, i);
		}
		gl.glUniformMatrix4fv(i, 1, true, value, 0);
	}
	
}
