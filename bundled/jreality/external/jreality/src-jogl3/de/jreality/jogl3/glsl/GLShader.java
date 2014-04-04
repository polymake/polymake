package de.jreality.jogl3.glsl;
import java.io.BufferedReader;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.nio.ByteBuffer;
import java.nio.IntBuffer;
import java.util.LinkedList;
import java.util.List;

import javax.media.opengl.GL3;

import de.jreality.jogl3.optimization.OptimizedGLShader;


/**
 * 
 * @author benjamin
 * loadShaderSrc() and attachShaders() copy pasted from Chris at
 * http://www.guyford.co.uk/showpage.php?id=50&page=How_to_setup_and_load_GLSL_Shaders_in_JOGL_2.0
 * 
 * the shader "in" and "uniform" variables must be each declared in its own line
 * comments in the shader source are only allowed as "//" at the very beginning of a line,
 * same applies for "#"
 *
 */
public class GLShader
{
	
	public static GLShader defaultPolygonShader = new GLShader("nontransp/polygon.v", "nontransp/polygon.f");
//	public static GLShader defaultCPolygonShader = new GLShader("nontransp/Cpolygon.v", "nontransp/Cpolygon.f");
//	public static GLShader defaultOPolygonShader = new OptimizedGLShader("../glsl/nontransp/polygon.v", "../glsl/nontransp/polygon.f");
	public static GLShader defaultPointShader = new GLShader("nontransp/pointNoSphere.v", "nontransp/pointNoSphere.f");
	public static GLShader defaultLineShader = new GLShader("nontransp/edge.v", "nontransp/edge.f");
	public static GLShader defaultPolygonLineShader = new GLShader("nontransp/tubeEdge.v", "nontransp/tubeEdge.f");
	public static GLShader defaultSphereShader = new GLShader("nontransp/sphere.v", "nontransp/sphere.f");
	public static GLShader defaultPointSphereShader = new GLShader("nontransp/spherePoint.v", "nontransp/spherePoint.f");
	//TODO defaultLineShader
	public static void initDefaultShaders(GL3 gl){

//		defaultCPolygonShader.init(gl);
//		defaultOPolygonShader.init(gl);
		defaultPolygonShader.init(gl);
		defaultPointShader.init(gl);
		defaultLineShader.init(gl);
		defaultPolygonLineShader.init(gl);
		defaultSphereShader.init(gl);
		defaultPointSphereShader.init(gl);
	}
	
	//represents a uniform or in variable in the shader
	public class ShaderVar{
		public ShaderVar(String name, String type){
			this.name = name;
			this.type = type;
		}
		private String name = "";
		public String getName() {
			return name;
		}
		private String type = "";
		public String getType() {
			return type;
		}
	}
	
	public int     vertexShaderProgram;
	public int     fragmentShaderProgram;
	public int     shaderprogram;
	private String vname, fname;
	protected String[] vsrc = null;
	protected String[] fsrc = null;
	
	public String[] getVSRC(){
		return vsrc;
	}
	public String[] getFSRC(){
		return fsrc;
	}
	
	public List<ShaderVar> shaderUniforms = new LinkedList<ShaderVar>();
	public List<ShaderVar> vertexAttributes = new LinkedList<ShaderVar>();
	
	public String getVertFilename(){
		return vname;
	}
	public String getFragFilename(){
		return fname;
	}
	
	public GLShader(String vert, String frag){
		vname = vert;
		fname = frag;
		loadVertexShaderSource(vert);
		loadFragmentShaderSource(frag);
	}
	
	public void init( GL3 gl )
	{
		try
		{
			attachShaders(gl);
		}
		catch (Exception e)
		{
			e.printStackTrace();
		}
	}

	private String[] loadShaderSrc( String name )
	{
		StringBuilder sb = new StringBuilder();
		try
		{
			//System.out.println(name);
			InputStream is = getClass().getResourceAsStream(name);
			BufferedReader br = new BufferedReader(new InputStreamReader(is));
			String line;
			while ((line = br.readLine()) != null)
			{
				sb.append(line);
				sb.append('\n');
			}
			is.close();
		}
		catch (Exception e)
		{
			e.printStackTrace();
		}
		//System.out.println("Shader is " + sb.toString());
		return new String[]{ sb.toString() };
	}
	
	/**
	 * includes the uniforms "projection" and "modelview"
	 * @param shaderSource
	 * @param target
	 */
	protected void findUniforms(String shaderSource, List<ShaderVar> target){
		findVars(shaderSource, new String[]{"uniform"}, target);
	}
	
	private void findVars(String source, String[] qualifiers, List<ShaderVar> target){
		String[] lines = source.split("\n");
		
		for(String l : lines){
			if((l.length() > 0 && l.substring(0,1).equals("#")) || (l.length()>1 && l.substring(0,2).equals("//")))
				continue;
			String[] words = l.split("\\s");
			
			
			//look for the first word named "in"
			int currentPos = 0;
			boolean cont = true;
			if(words.length == 0)
				cont = false;
			boolean foundVar = false;
			while(cont){
				boolean isQualifier = false;
				for(String q : qualifiers){
					if(words[currentPos].equals(q))
						isQualifier = true;
				}
				if(isQualifier){
					cont = false;
					foundVar = true;
				}else{
					currentPos++;
					if(currentPos == words.length)
						cont = false;
				}
			}
			String type = "";
			String name = "";
			//if the line contains a qualifier
			if(foundVar){
				//carry on searching for the next word, which must be the variable type
				currentPos++;
				cont = true;
				while(cont){
					if(!words[currentPos].equals("")){
						type = words[currentPos];
						cont = false;
					}
					currentPos++;
				}
				cont = true;
				while(cont){
					if(!words[currentPos].equals("")){
						if(';' == words[currentPos].charAt(words[currentPos].length()-1)){
							name = words[currentPos].substring(0, words[currentPos].length()-1);
						}else{
							name = words[currentPos];
						}
						cont = false;
					}
					currentPos++;
				}
				
//				System.out.println("type = " + type);
//				System.out.println("name = " + name);
				//this is needed, when instanced rendering lines,
				//where we want to call the second endpoint "_vertex_coordinates"
				if(name.charAt(0) != '_'){
					target.add(new ShaderVar(name, type));
				}
			}
		}
	}
	
	public void findVertexAttributes(){
//		System.out.println("vertex attributes");
		findVars(vsrc[0], new String[]{"in", "attribute", "varying"}, vertexAttributes);
	}
	
	private void loadVertexShaderSource(String name){
		vsrc = loadShaderSrc(name);
//		System.out.println("vertex shader uniforms");
		findUniforms(vsrc[0], this.shaderUniforms);
		findVertexAttributes();
	}
	private void loadFragmentShaderSource(String name){
		fsrc = loadShaderSrc(name);
//		System.out.println("fragment shader uniforms");
		findUniforms(fsrc[0], this.shaderUniforms);
	}

	private void attachShaders( GL3 gl ) throws Exception
	{
		shaderprogram = gl.glCreateProgram();
		if(vsrc != null){
			vertexShaderProgram = gl.glCreateShader(GL3.GL_VERTEX_SHADER);
			gl.glShaderSource(vertexShaderProgram, 1, vsrc, null, 0);
			gl.glCompileShader(vertexShaderProgram);
			IntBuffer intbuf = IntBuffer.allocate(1000);
			ByteBuffer bytebuf = ByteBuffer.allocate(1000);
			gl.glGetShaderInfoLog(vertexShaderProgram, 1000, intbuf, bytebuf);
			byte[] dst = new byte[intbuf.get(0)];
			bytebuf.get(dst);
			String message = new String(dst);
			if(message.length() != 0)
				System.err.println(vname + " " + message);
			gl.glAttachShader(shaderprogram, vertexShaderProgram);
		}
		if(fsrc != null){
			fragmentShaderProgram = gl.glCreateShader(GL3.GL_FRAGMENT_SHADER);
			gl.glShaderSource(fragmentShaderProgram, 1, fsrc, null, 0);
			gl.glCompileShader(fragmentShaderProgram);
			IntBuffer intbuf = IntBuffer.allocate(1000);
			ByteBuffer bytebuf = ByteBuffer.allocate(1000);
			gl.glGetShaderInfoLog(fragmentShaderProgram, 1000, intbuf, bytebuf);
			byte[] dst = new byte[intbuf.get(0)];
			bytebuf.get(dst);
			String message = new String(dst);
			if(message.length() != 0)
				System.err.println(fname + " " + message);
			gl.glAttachShader(shaderprogram, fragmentShaderProgram);
		}
		
		gl.glLinkProgram(shaderprogram);
		gl.glValidateProgram(shaderprogram);
		IntBuffer intBuffer = IntBuffer.allocate(1);
		gl.glGetProgramiv(shaderprogram, GL3.GL_LINK_STATUS, intBuffer);
		if (intBuffer.get(0) != 1)
		{
			gl.glGetProgramiv(shaderprogram, GL3.GL_INFO_LOG_LENGTH, intBuffer);
			int size = intBuffer.get(0);
			System.err.println("Program link error: ");
			if (size > 0)
			{
				ByteBuffer byteBuffer = ByteBuffer.allocate(size);
				gl.glGetProgramInfoLog(shaderprogram, size, intBuffer, byteBuffer);
				for (byte b : byteBuffer.array())
				{
					System.err.print((char) b);
				}
			}
			else
			{
				System.out.println("Unknown");
			}
			System.exit(1);
		}
	}
	
    public void useShader( GL3 gl )
	{
		gl.glUseProgram(shaderprogram);
	}

	public void dontUseShader( GL3 gl )
	{
		gl.glUseProgram(0);
	}
}