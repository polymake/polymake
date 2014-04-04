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


package de.jreality.renderman;

import java.awt.Color;
import java.awt.geom.Rectangle2D;
import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.io.PrintWriter;
import java.util.Map;

import de.jreality.renderman.shader.RendermanShader;

/**
 * The methods in this class typically stand in 1:1 relationship to possible RIB commands.
 * @author <a href="mailto:hoffmann@math.tu-berlin.de">Tim Hoffmann</a>, Charles Gunn
 *
 */
public class Ri {

    public  static final int BEZIER_BASIS = 0;
    public  static final int BSPLINE_BASIS = 1;
    public  static final int CATMULL_ROM_BASIS = 2;
    public  static final int HERMITE_BASIS = 3;
    public  static final int POWER_BASIS = 4;
    private  PrintWriter w;
    private  int lightCount;
    
    public   void begin(String name) {
        try {
             w =new PrintWriter(new FileWriter(new File(name)));
            
        } catch (IOException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }
        lightCount=0;
    }
    public   void begin(File file) {
        try {
            w =new PrintWriter(new FileWriter(file));
        } catch (IOException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }
        lightCount=0;
    }
    public   void end() {
        w.close();
    }
    
    public  void verbatim(String s)	{
    	w.println(s);
    }
    public  void comment(String s) {
        String[] ss = s.split("\\n");
        for (int i = 0; i < ss.length; i++) {
            w.println("# "+ss[i]);
        }
    }
    
    public String quote(String s)	{
    	return("\""+s+"\"");
    }
    public  void declare(String name, String type)	{
        w.println("Declare "+RIBHelper.str(name)+" "+RIBHelper.str(type));    	
    }
    public  void option(String name, Map map) {
        w.print("Option "+RIBHelper.str(name)+" ");
        RIBHelper.writeMap(w, map);
     }
    
    public  void attribute(String name, Map map) {
      w.print("Attribute "+RIBHelper.str(name)+" ");
      RIBHelper.writeMap(w, map);
      
  }

    public  void display(String name, String type, String mode, Map map ) {
        w.print("Display "+RIBHelper.str(name)+" "+RIBHelper.str(type)+" "+RIBHelper.str(mode)+" ");
        RIBHelper.writeMap(w,map);
    }
    
    public   void format(int xresolution, int yresolution, float pixelaspectratio) {
        w.println("Format "+xresolution+" "+yresolution+" "+pixelaspectratio);
    }
    
    public   void shadingRate(float rate) {
        w.println("ShadingRate "+rate);
    }
    
	public  void clipping(double near, double far) {
		w.println("Clipping "+near+" "+far);
	}
	
	public  void depthOfField(double fstop, double flength, double fdistance) {
		if (fstop < 0) return;		// no depth of field
		w.println("DepthOfField "+fstop+" "+flength+" "+fdistance);
	}
	
	public  void screenWindow(Rectangle2D sw)	{
		w.println("ScreenWindow ["+sw.getMinX()+" "+ sw.getMaxX()+" "+sw.getMinY()+" "+sw.getMaxY()+"]");
	}
	public  void projection(String name, Map map) {
        w.print("Projection "+RIBHelper.str(name)+" ");
        RIBHelper.writeMap(w,map);
    }
    
    public  int lightSource(String name, Map map) {
        lightCount++;
        w.print("LightSource "+RIBHelper.str(name)+" "+lightCount+" ");
        RIBHelper.writeMap(w,map);
        return lightCount;
    }
//    public   int lightSource(String name, String[] tokens, Object[] values);
    
    public   void worldBegin() {
        w.println("WorldBegin");
    }
    public   void worldEnd() {
        w.println("WorldEnd");
    }
    public   void frameBegin(int n) {
        w.println("FrameBegin "+n);
    }
    public   void frameEnd() {
        w.println("FrameEnd");
    }
    public   void attributeBegin() {
        w.println("AttributeBegin");
    }
    public   void attributeBegin(String name) {
        w.println("AttributeBegin # "+name);
    }
    public   void attributeEnd() {
        w.println("AttributeEnd");
    }
	public void attributeEnd(String name) {
        w.println("AttributeEnd # "+name);
	}
    public   void transformBegin() {
        w.println("TransformBegin");
    }
    public   void transformEnd() {
        w.println("TransformEnd");
    }
    public   void archiveBegin(String name) {
        w.println("ArchiveBegin " +RIBHelper.str(name));
    }
    public   void archiveEnd() {
        w.println("ArchiveEnd");
    }
	public  void readArchive(String foo) {
		w.println("ReadArchive "+RIBHelper.str(foo));
	}

    public  void surface(String name, Map map) {
        w.print("Surface "+RIBHelper.str(name)+" ");
        RIBHelper.writeMap(w,map);
    }
    
    public  void displacement(String name, Map map) {
      w.print("Displacement "+RIBHelper.str(name)+" ");
      RIBHelper.writeMap(w,map);
    }
    
    public void interior(String name, Map map){
      w.print("Interior "+RIBHelper.str(name)+" ");
      RIBHelper.writeMap(w,map);
    }
    public void exterior(String name, Map map){
      w.print("Exterior "+RIBHelper.str(name)+" ");
      RIBHelper.writeMap(w,map);
    }    
    public void atmosphere(String name, Map map){
      w.print("Atmosphere "+RIBHelper.str(name)+" ");
      RIBHelper.writeMap(w,map);
    }   
    
    public  void imager(String name, Map map) {
        w.print("Imager "+RIBHelper.str(name)+" ");
        RIBHelper.writeMap(w,map);
    }
 
    public   void color(Color color) {
        float[] cc = color.getRGBComponents(null);
        color(cc[0], cc[1], cc[2]);
        if (cc.length == 4) opacity(cc[3]);
    }
    
    public   void color(double[] color) {
    	color((float) color[0], (float) color[1], (float) color[2]);
    	if (color.length == 4) opacity((float) color[3]);
    }
    
   public  void color(float r, float g, float b){
    	color( new float[]{r,g,b});
    }
    public   void color(float[] color) {
        w.print("Color ");
       if (color.length == 3) RIBHelper.writeObject(w,color);
       else RIBHelper.writeObject(w, new float[]{color[0], color[1], color[2]});
        w.println("");
       if (color.length == 4) opacity(color[3]);
    }
    
    public   void opacity(float[] color) {
        w.print("Opacity ");
        RIBHelper.writeObject(w,color);
        w.println("");
    }
    
    public   void opacity(float color) {
        w.print("Opacity ");
        float[] opa = {color, color, color};
        RIBHelper.writeObject(w, opa);
        w.println("");
    }
    
   public   void concatTransform(float[] transform) {
        w.print("ConcatTransform ");
        RIBHelper.writeObject(w,transform);
        w.println("");
    }
    public   void transform(float[] transform) {
        w.print("Transform ");
        RIBHelper.writeObject(w,transform);
        w.println("");
    }
    public   void identity() {
        w.println("Identity");
    }
      
    public  void sphere(float radius, float zmin, float zmax, float thetamax, Map map) {
        w.print("Sphere "+radius+" "+zmin+" "+zmax+" "+thetamax+" ");
        RIBHelper.writeMap(w,map);
    }
    
//    public   void sphere(float radius, float zmin, float zmax, float thetamax, String[] tokens, Object[] values) ;

    public  void cylinder(float radius, float zmin, float zmax, float thetamax, Map map) {
        w.print("Cylinder "+radius+" "+zmin+" "+zmax+" "+thetamax+" ");
        RIBHelper.writeMap(w,map);
    }
    
    public  void disk(float z, float radius, float thetamax, Map map) {
        w.print("Disk "+z+" "+radius+" "+thetamax+" ");
        RIBHelper.writeMap(w,map);
    }
    
    public  void clippingPlane(float x, float y, float z, float nx, float ny, float nz) {
        w.print("ClippingPlane "+x+" "+y+" "+z+" "+nx+" "+ny+" "+nz+"\n");
    }

    public  void points(int npoints, Map map) {
        w.print("Points ");
        RIBHelper.writeMap(w,map);
    }
    
    public  void pointsPolygons(int npolys,int[] nvertices,int[] vertices,Map map) {
        w.print("PointsPolygons ");
        RIBHelper.writeObject(w,nvertices);
        w.print(" ");
        RIBHelper.writeObject(w,vertices);
        w.print(" ");
        RIBHelper.writeMap(w,map);
    }
   
    public  void patchMesh(String type, int nu,  boolean uwrap, int nv, boolean vwrap, Map map) {
        w.print("PatchMesh "+quote(type)+" "+nu+" "+quote(uwrap ? "periodic" : "nonperiodic")+" "+nv+" "+
        		quote(uwrap ? "periodic" : "nonperiodic"));
//        map.remove("N");
        map.remove("uniform normal N");
        RIBHelper.writeMap(w,map);
    }
    
    public  void patch(String type, Map map)	{
    	w.print("Patch ");
    	w.print(RIBHelper.str(type)+" ");
    	RIBHelper.writeMap(w,map);
    }
    public  void curves(String type, int[] nvertices, String wrap, Map map) {
        w.print("Curves "+RIBHelper.str(type)+" ");
        RIBHelper.writeObject(w,nvertices);
        w.print(" "+RIBHelper.str(wrap)+" ");
        RIBHelper.writeMap(w,map);
    }
    
    public   void basis(int ubasis, int ustep, int vbasis, int vstep) {
        w.print("Basis "+ubasis+" "+ustep+" "+vbasis+" "+vstep);
    }
    
	public  void shader(RendermanShader sh) {
        w.print(sh.getType()+" "+RIBHelper.str(sh.getName())+" ");
        RIBHelper.writeMap(w,sh.getAttributes());
	}
}
