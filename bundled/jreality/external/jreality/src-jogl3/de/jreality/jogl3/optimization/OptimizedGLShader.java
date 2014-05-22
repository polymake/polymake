package de.jreality.jogl3.optimization;

import java.util.LinkedList;

import javax.media.opengl.GL3;

import de.jreality.jogl3.glsl.GLShader;
import de.jreality.jogl3.glsl.GLShader.ShaderVar;

public class OptimizedGLShader extends GLShader {
	
	private LinkedList<String[]> VertUniforms;
	private LinkedList<String[]> FragUniforms;
	
	public LinkedList<String[]> getFragUniforms(){
		return FragUniforms;
	}
	
	public LinkedList<String[]> getVertUniforms(){
		return VertUniforms;
	}
	
	private int offset = 0;
	
	//TODO remove this override method. It's only for testing
//	public void init(GL3 gl){
//		
//		GLShader sh = new GLShader("../glsl/nontransp/Cpolygon.v", "../glsl/nontransp/Cpolygon.f");
//		this.vsrc = sh.getVSRC();
//		this.fsrc = sh.getFSRC();
//		super.init(gl);
//	}
	
	public int getNumFloatsNecessary(){
		return numFloatsNecessary;
		
	}
	private int numFloatsNecessary = 0;
	
	public OptimizedGLShader(String v, String f){
		super(v,f);
		VertUniforms = new LinkedList<String[]>();
		FragUniforms = new LinkedList<String[]>();
		findUniformsAndReplace(vsrc, VertUniforms, true);
//		System.out.println(vsrc[0]);
		findUniformsAndReplace(fsrc, FragUniforms, false);
//		System.out.println(fsrc[0]);
		
		if(offset%4==0)
			numFloatsNecessary =  offset;
		else
			numFloatsNecessary = 4*(offset/4) + 4;
		
		this.shaderUniforms = new LinkedList<GLShader.ShaderVar>();
		findUniforms(vsrc[0], this.shaderUniforms);
		
		findUniforms(fsrc[0], this.shaderUniforms);
	}
	
	private void findUniformsAndReplace(String[] source, LinkedList<String[]> uniforms, boolean isVertexShader) {
//		System.err.println("uniforms:");
//		for(String[] s : uniforms)
//			System.err.println(s[1]);
//		System.err.println("uniforms ende.");
		
		
		String[] qualifiers = new String[]{"uniform"};
		String[] lines = source[0].split("\n");
		
		for(int i = 0; i < lines.length; i++){
			String l = lines[i];
			if((l.length() > 0 && l.substring(0,1).equals("#")) || (l.length()>1 && l.substring(0,2).equals("//")))
				continue;
			String[] words = l.split("\\s");
			
			
			//look for the first word named "in" or "uniform" respectively
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
			//if the line contained a qualifier at the first position,...
			if(foundVar){
				//...then carry on searching for the next word, which must be the variable type
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
				if(!(name.length() >= 4 && name.substring(0, 4).equals("sys_")) && name.charAt(0) != '_' && !name.equals("projection") && !name.equals("_inverseCamRotation") && !name.equals("textureMatrix") && !type.equals("sampler2D") && !name.equals("has_Tex") && !name.equals("has_reflectionMap")){
					//TODO and remove from source!
					lines[i] = "";
					uniforms.add(new String[]{type, name});
				}
			}
		}
		source[0] = "";
		for(String l : lines){
			if(l.length() >= 15 && l.substring(0, 15).equals("void main(void)")){
				source[0] += "void main(void){\n";
				//add everything else...
				if(isVertexShader)
					source[0] += "instanceID = vertex_id;\n";
				
				if (offset%4 != 0)
					offset = 4*(offset/4) + 4;
				//sort uniforms to get mat4 and vec4 to the front!!!
				for(int i = 0; i < uniforms.size(); i++){
					if(uniforms.get(i)[0].equals("vec4")){
						String[] rem = uniforms.remove(i);
						uniforms.addFirst(rem);
					}
				}for(int i = 0; i < uniforms.size(); i++){
					if(uniforms.get(i)[0].equals("mat4")){
						String[] rem = uniforms.remove(i);
						uniforms.addFirst(rem);
					}
				}
				for(String[] s : uniforms){
					if(s[0].equals("mat4")){
						source[0] += s[1] + " = mat4(" + this.texel(offset/4, isVertexShader) + ", "
								+ this.texel((offset)/4+1, isVertexShader) + ", "
								+ this.texel((offset)/4+2, isVertexShader) + ", "
								+ this.texel((offset)/4+3, isVertexShader) + ");\n";
						offset += 16;
					}else if(s[0].equals("vec4")){
						source[0] += s[1] + " = " + this.texel(offset/4, isVertexShader) + ";\n";
						offset += 4;
					}else if(s[0].equals("float")){
						//no of texel is offset/4
						//coordinate from texel is offset%4
						source[0] += s[1] + " = " + this.texel(offset/4, isVertexShader) + "[" + offset%4 + "];\n";
						offset += 1;
					}else if(s[0].equals("int") && !s[1].equals("has_reflectionMap")){
						source[0] += s[1] + " = " + this.floatToInt(this.texel(offset/4, isVertexShader) + "[" + offset%4 + "]") + ";\n";
						offset += 1;
					}else if(s[0].equals("vec2")){
						source[0] += s[1] + " = vec2(" + this.texel(offset/4, isVertexShader) + "[" + offset%4 + "], "
																	+ this.texel((offset+1)/4, isVertexShader) + "[" + (offset+1)%4 + "]);\n";
						offset += 2;
					}else if(s[0].equals("vec3")){
						source[0] += s[1] + " = vec3(" + this.texel(offset/4, isVertexShader) + "[" + offset%4 + "], "
								+ this.texel((offset+1)/4, isVertexShader) + "[" + (offset+1)%4 + "], "
								+ this.texel((offset+2)/4, isVertexShader) + "[" + (offset+2)%4 + "]);\n";
						offset += 3;
					}
				}
				
				source[0] += "\n";
				source[0] += l.substring(15);
			}else{
				source[0] += l;
				source[0] += "\n";
				//append in front the line "uniform sampler2D uniforms
				if(l.length() >= 8 && l.substring(0, 8).equals("#version")){
					if(isVertexShader){
						source[0] += "flat out float instanceID;\n";
						source[0] += "in float vertex_id;\n";
					}else
						source[0] += "flat in float instanceID;\n";
					source[0] += "uniform sampler2D uniforms;";
					source[0] += "\n";
					for(String[] s : uniforms){
						if(!s[1].equals("has_reflectionMap"))
							source[0] += s[0] + " " + s[1] + ";\n";
					}
				}
			}
			
		}
		source[0] += "}\n";
	}
	
	private String texel(int line, boolean vert){
		return "texelFetch(uniforms, ivec2(" + line + ", instanceID), 0)";
	}
	private String floatToInt(String s){
		return "floatBitsToInt(" + s + ")";
	}
}
