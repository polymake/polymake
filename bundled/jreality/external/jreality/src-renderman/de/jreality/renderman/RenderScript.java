package de.jreality.renderman;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.Writer;
import java.util.HashSet;

import de.jreality.shader.Texture2D;

class RenderScript {
	
  private boolean display=true;
  private boolean execute=false;
  private boolean writeToFile=true;
  
	final File dir;
	
	final String ribFileName, texCmd, shaderCmd, refMapCmd, renderer, texSuffix, refMapSuffix;
	
  final int type;
  
	HashSet<String> ribFiles=new HashSet<String>(),
			         shaders=new HashSet<String>();
  HashSet<String[]> textures=new HashSet<String[]>(),
  					reflectionMaps=new HashSet<String[]>();


	
	
	protected RenderScript(File dir, String ribFileName, int type) {
		this.dir=dir;
		this.ribFileName=ribFileName;
    this.type=type;
		switch (type) {
		case RIBViewer.TYPE_AQSIS:
			texCmd="teqser ";
			shaderCmd="aqsl ";
			refMapCmd="teqser -envcube ";
			renderer="aqsis ";
			texSuffix=".tex";
			refMapSuffix=".env";
			break;
		case RIBViewer.TYPE_3DELIGHT:
			texCmd="tdlmake ";
			shaderCmd="shaderdl ";
			refMapCmd="tdlmake -envcube ";
			renderer="renderdl ";
			texSuffix=".tex";
			refMapSuffix=".env";
			break;
    case RIBViewer.TYPE_PIXIE:
      texCmd="texmake ";
      shaderCmd="sdrc ";
      refMapCmd="texmake -envcube ";
      renderer="rndr ";
      texSuffix=".tex";
      refMapSuffix=".env";
      break;
		default:
			texCmd="txmake -resize 'up-' ";
			shaderCmd="shader ";
			refMapCmd="txmake -envcube ";
			renderer="prman ";
			texSuffix=".tex";
			refMapSuffix=".env";
		}
	}
	
	void addTexture(String tex, int smode, int tmode) {   
	    String repeatS=type == RIBViewer.TYPE_AQSIS ? "-swrap " : "-smode ";      
	    switch(smode){
	    //case Texture2D.GL_MIRRORED_REPEAT: repeatS="";  // <- no support in renderman
	    case(Texture2D.GL_REPEAT): repeatS+="'periodic' "; break;           
	    case(Texture2D.GL_CLAMP): repeatS+="'clamp' "; break;                 
	    case(Texture2D.GL_CLAMP_TO_EDGE): repeatS+="'clamp' "; break;
	    default: repeatS+="'periodic' ";
	    }    
	    String repeatT=type == RIBViewer.TYPE_AQSIS ? "-twrap " : "-tmode ";
	    switch(tmode){
	    //case Texture2D.GL_MIRRORED_REPEAT: repeatS="";  // <- no support in renderman
	    case(Texture2D.GL_REPEAT): repeatT+="'periodic' "; break;           
	    case(Texture2D.GL_CLAMP): repeatT+="'clamp' "; break;               
	    case(Texture2D.GL_CLAMP_TO_EDGE): repeatT+="'clamp' "; break;
	    default: repeatT+="'periodic' ";
	    }    
	   	textures.add(new String[]{tex, repeatS, repeatT});
	}
	
	void addReflectionMap(String... reflectionMap) {
		reflectionMaps.add(reflectionMap);
	}

	void addShader(String shader) {
		shaders.add(shader);
	}

	void addRibFile(String ribFile) {
		ribFiles.add(ribFile);
	}
	
	void finishScript() {
    
    String script=""; 
    String cmd;
    final String separator="\n";
    
    for (String[] texName : textures) {
      cmd = texCmd+texName[1]+texName[2]+ribFileName+texName[0]+".tiff "+ribFileName+texName[0]+texSuffix;
      script=script+separator+cmd;
      exec(cmd, true);
    } 
    
    for (String shaderName : shaders) {
      cmd = shaderCmd+shaderName;
      script=script+separator+cmd;
      exec(cmd, true);
    }
    
    for (String[] refMap : reflectionMaps) {
      cmd = refMapCmd+refMap[1]+" "+refMap[2]+" "+refMap[3]+" "+refMap[4]+" "+refMap[5]+" "+refMap[6]+" "+ribFileName+refMap[0]+refMapSuffix;
      script=script+separator+cmd;
      exec(cmd, true);
    }
    
    cmd = renderer + ribFileName;
    if(display&&(type!=RIBViewer.TYPE_PIXAR)&&(type!=RIBViewer.TYPE_PIXIE)) cmd = renderer +"-d "+ ribFileName +" &";
    script=script+separator+cmd;
    exec(cmd, false);

    if(display&&((type==RIBViewer.TYPE_PIXAR)||(type==RIBViewer.TYPE_PIXIE))){
      String fileName=ribFileName.substring(0,ribFileName.length()-4); 
      cmd="display "+ fileName+".tif &";  
      script=script+separator+cmd;
      exec(cmd, false);
    } 
    
    if(writeToFile) writeToFile(script);    
    else dumpScript(script);      
  }
  
  private void writeToFile(String script){
    
    String scriptName=ribFileName+"_renderScript.bat";
    
    try {      
      Writer f1 = new FileWriter(dir.getAbsolutePath()+"/"+scriptName);
      BufferedWriter f2 = new BufferedWriter(f1);
      f2.write(script);    
      f2.close();
      f1.close();
    } catch (IOException e) {
      System.err.println("can't write "+scriptName);
    }   
    
    //dumpScript("sh "+scriptName);
  }
    
    
  private void dumpScript(String script){		
		System.out.println("========= render script ==========\n\n");	
    System.out.println("cd "+dir.getAbsolutePath());   
    System.out.println(script);
		System.out.println("\n\n========= render script ==========\n\n");
	}

	private void exec(String cmd, boolean wait) {
		if (!execute) return;
		ProcessBuilder pb = new ProcessBuilder(cmd.split(" "));
		pb.directory(dir);
		pb.redirectErrorStream(true);
		try {
			final Process proc = pb.start();
			Thread t = new Thread(new Runnable() {
				public void run() {
					BufferedReader br = new BufferedReader(new InputStreamReader(proc.getInputStream()));
					String line = null;
					try {
						while ((line = br.readLine()) != null) System.out.println(line);
					} catch (IOException e) {
						// TODO Auto-generated catch block
						e.printStackTrace();
					}
				}
			});
			t.start();
			if (wait) proc.waitFor();
		} catch (InterruptedException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
	}
	
	
	
}
