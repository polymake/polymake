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


package de.jreality.soft;

import java.awt.Color;
import java.awt.Font;
import java.util.Arrays;
import java.util.logging.Logger;

import de.jreality.backends.label.LabelUtility;
import de.jreality.scene.Appearance;
import de.jreality.scene.Cylinder;
import de.jreality.scene.DirectionalLight;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.IndexedLineSet;
import de.jreality.scene.PointLight;
import de.jreality.scene.PointSet;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SceneGraphVisitor;
import de.jreality.scene.Sphere;
import de.jreality.scene.SpotLight;
import de.jreality.scene.Transformation;
import de.jreality.scene.data.Attribute;
import de.jreality.scene.data.AttributeEntityUtility;
import de.jreality.scene.data.DataList;
import de.jreality.scene.data.DoubleArray;
import de.jreality.scene.data.DoubleArrayArray;
import de.jreality.scene.data.IntArray;
import de.jreality.scene.data.IntArrayArray;
import de.jreality.shader.CommonAttributes;
import de.jreality.shader.DefaultTextShader;
import de.jreality.shader.EffectiveAppearance;
import de.jreality.shader.ImageData;
import de.jreality.shader.ShaderUtility;

/**
 * This class traverses a scene graph starting from the given "root" scene
 * graph component. The children of each sgc are visited in the following
 * order: First the appearance is visited then the current transformation
 * is popped to the transformationstack and a copy of it gets multiplied by
 * the transformation of the sgc. This copy is then visited. Then all
 * geometries are visited. 
 * Finally the transformation gets poped from the stack.
 * @version 1.0
 * @author <a href="mailto:hoffmann@math.tu-berlin.de">Tim Hoffmann</a>
 *
 */
public class RenderTraversal extends SceneGraphVisitor {
    private static final String FACE_SHADER = /*"faceShader."+*/CommonAttributes.POLYGON_SHADER;
  private boolean shaderUptodate;

  protected Environment     environment = new Environment();

  protected EffectiveAppearance eAppearance;

  double[]  initialTrafo,   currentTrafo;
  private   Transformation  initialTransformation;
  protected LineShader   lineShader;
  protected PolygonPipeline pipeline;
  //protected PolygonShader   pointOutlineShader;
  protected PointShader   pointShader;
  protected PolygonShader   polygonShader;
  protected RenderTraversal reclaimableSubcontext;

  /**
   * 
   */
  public RenderTraversal() {
    super();
    eAppearance=EffectiveAppearance.create();
  }

  protected RenderTraversal(RenderTraversal parentContext) {
    eAppearance=parentContext.eAppearance;
    initializeFromParentContext(parentContext);
  }

  /**
  	 * @return PolygonPipeline
  	 */
  public PolygonPipeline getPipeline() {
    return pipeline;
  }

  /**
     * Sets the pipeline.
     * @param pipeline The pipeline to set
     */
  public void setPipeline(PolygonPipeline pipeline) {
    if (this.pipeline != null)
      this.pipeline.setEnvironment(null);
    this.pipeline=pipeline;
    pipeline.setEnvironment(environment);
  }

  protected void initializeFromParentContext(RenderTraversal parentContext) {
    RenderTraversal p=parentContext;
    environment=p.environment;
    eAppearance=parentContext.eAppearance;
    pipeline=p.pipeline;
    pipeline.setFaceShader(polygonShader=p.polygonShader);
    pipeline.setLineShader(lineShader=p.lineShader);
    pipeline.setPointShader(pointShader=p.pointShader);
	shaderUptodate= p.shaderUptodate;
    //pipeline.setPointOutlineShader(pointOutlineShader=p.pointOutlineShader);
    pipeline.setMatrix(currentTrafo=initialTrafo=parentContext.currentTrafo);
  }

  /**
   * Sets the initialTransformation.
   * @param initialTransformation The initialTransformation to set
   */
  public void setInitialTransformation(Transformation initialTransformation) {
    this.initialTransformation= initialTransformation;
    environment.setInitialTransformation(initialTransformation);
  }

  RenderTraversal subContext() {
    if (reclaimableSubcontext != null) {
      reclaimableSubcontext.initializeFromParentContext(this);
      //TODO is this o.k. ?
	  //reclaimableSubcontext.shaderUptodate = false;
//	  reclaimableSubcontext.shaderUptodate = this.shaderUptodate;
//	  reclaimableSubcontext.pointShader   = this.pointShader;
//	  reclaimableSubcontext.lineShader    = this.lineShader;
//	  reclaimableSubcontext.polygonShader = this.polygonShader;
      return reclaimableSubcontext;
    } else
      return reclaimableSubcontext= new RenderTraversal(this);
  }
  /**
   * This starts the traversal of a SceneGraph starting form root.
   * @param root
   */
  public void traverse(SceneGraphComponent root) {
    environment.removeAll();
    if (initialTrafo == null)
      initialTrafo= new double[16];
    if (initialTransformation != null)
      initialTransformation.getMatrix(initialTrafo);
    else
      VecMat.assignIdentity(initialTrafo);
    currentTrafo= initialTrafo;
    environment.traverse(root);
    root.accept(this);
    pipeline.setMatrix(initialTrafo);
  }

  public void visit(SceneGraphComponent c) {
    if(c.isVisible())
        c.childrenAccept(subContext());
  }

  public void visit(Transformation t) {
    if (initialTrafo == currentTrafo)
      currentTrafo= new double[16];
    VecMat.copyMatrix(initialTrafo, currentTrafo);
    VecMat.multiplyFromRight(currentTrafo, t.getMatrix());
    pipeline.setMatrix(currentTrafo);
  }

  public void visit(Appearance app) {
    eAppearance = eAppearance.create(app);
    shaderUptodate = false;
  }
  private void setupShader()
  {
    if(!eAppearance.getAttribute("geometryShader", "default").equals("default"))
      Logger.getLogger("de.jreality").warning("unsupported geometry shader");
    String geomShaderName = (String)eAppearance.getAttribute("geometryShader.name", "");
    
    if((boolean)eAppearance.getAttribute(ShaderUtility.nameSpace(geomShaderName, CommonAttributes.FACE_DRAW), CommonAttributes.FACE_DRAW_DEFAULT)) {
        PolygonShader fs=ShaderLookup
            .getPolygonShaderAttr(eAppearance, geomShaderName, FACE_SHADER);
        pipeline.setFaceShader(this.polygonShader=fs);
    } else {
        pipeline.setFaceShader(this.polygonShader=null);
    }
    if((boolean)eAppearance.getAttribute(ShaderUtility.nameSpace(geomShaderName, CommonAttributes.EDGE_DRAW), CommonAttributes.EDGE_DRAW_DEFAULT)) {
        LineShader ls=ShaderLookup
       .getLineShaderAttr(eAppearance, geomShaderName, CommonAttributes.LINE_SHADER);;
       pipeline.setLineShader(this.lineShader=ls);
    } else {
        pipeline.setLineShader(this.lineShader=null);
    }
       
    if((boolean)eAppearance.getAttribute(ShaderUtility.nameSpace(geomShaderName, CommonAttributes.VERTEX_DRAW), CommonAttributes.VERTEX_DRAW_DEFAULT)) {
        PointShader ps=ShaderLookup
            .getPointShaderAttr(eAppearance, geomShaderName, CommonAttributes.POINT_SHADER);;
        pipeline.setPointShader(this.pointShader=ps);
    } else {
        pipeline.setPointShader(this.pointShader=null);
    }
    //pipeline.setPointRadius((double)eAppearance.getAttribute(geomShaderName, .5));
//    pipeline.setPointOutlineShader(this.pointOutlineShader=pos);

//      pipeline.setPointOutlineShader(pointOutlineShader);
//      pipeline.setPointRadius(psa.getPointSize());
//      pipeline.setOutlineFraction(psa.getOutlineFraction());
//      pipeline.setLineWidth(lsa.getLineWidth());
    shaderUptodate = true;
  }

//  public void visit(Geometry g) {
//    System.err.println("Warning: unknown geometry type " + g);
//  }

  public void visit(IndexedLineSet g) {
    if(!shaderUptodate) setupShader();
    DataList dl  = g.getEdgeAttributes(Attribute.INDICES);
    if(lineShader != null&& dl!= null) {
        IntArrayArray edgeIndices=dl.toIntArrayArray();
        DoubleArrayArray vertices=g.getVertexAttributes(Attribute.COORDINATES)
          .toDoubleArrayArray();
        pipeline.startGeometry(g);
        for (int i= 0, n=edgeIndices.size(); i < n; i++)
        {
          IntArray edge=edgeIndices.item(i).toIntArray();
          for(int j = 0; j<edge.getLength()-1;j++) {
              DoubleArray p1=vertices.item(edge.getValueAt(j)).toDoubleArray();
              DoubleArray p2=vertices.item(edge.getValueAt(j+1)).toDoubleArray();
              //pipeline.processLine(p1, p2);
              pipeline.processPseudoTube(p1, p2);
          }
          //int ix1=edge.getValueAt(0), ix2=edge.getValueAt(1);
          //DoubleArray p1=vertices.item(ix1).toDoubleArray();
          //DoubleArray p2=vertices.item(ix2).toDoubleArray();
          //pipeline.processLine(p1, p2);
        }
        // Labels
        if (g.getEdgeAttributes(Attribute.LABELS) != null) {
          Class shaderType =  (Class) eAppearance.getAttribute(
              ShaderUtility.nameSpace(CommonAttributes.LINE_SHADER,"textShader"),DefaultTextShader.class);
          
          DefaultTextShader ts = (DefaultTextShader) AttributeEntityUtility.createAttributeEntity(
              shaderType,
              ShaderUtility.nameSpace(CommonAttributes.LINE_SHADER,"textShader"),
              eAppearance);
          Font font = ts.getFont();
          Color c = ts.getDiffuseColor();
          double scale = ts.getScale().doubleValue();
          double[] offset = ts.getOffset();
          int alignment = ts.getAlignment();
          
          renderLabels(scale, offset, alignment, LabelUtility.createEdgeImages(g, font, c), g.getVertexAttributes(Attribute.COORDINATES).toDoubleArrayArray(), g.getEdgeAttributes(Attribute.INDICES).toIntArrayArray());
        }
    }
    visit((PointSet)g);
  }
  
  private int[] fni =new int[Polygon.VERTEX_LENGTH];
  private IntArray fnia =new IntArray(fni);
  
  public void visit(IndexedFaceSet ifs) {
    if(!shaderUptodate) setupShader();

    if(polygonShader != null) {
        DataList indices=ifs.getFaceAttributes(Attribute.INDICES);
        if(indices != null){
        DoubleArrayArray points
          =ifs.getVertexAttributes(Attribute.COORDINATES).toDoubleArrayArray();
    	DoubleArrayArray normals =null;
    	DataList vndl =ifs.getVertexAttributes(Attribute.NORMALS);
        if(vndl !=null)
            normals =vndl.toDoubleArrayArray();
      	DataList texCoords = (ifs.getVertexAttributes(Attribute.TEXTURE_COORDINATES));
        pipeline.startGeometry(ifs);

        if(normals== null || polygonShader instanceof FlatPolygonShader) {
            DoubleArrayArray faceNormals = null;
            DataList fndl =ifs.getFaceAttributes(Attribute.NORMALS);
            if(fndl !=null) faceNormals = fndl.toDoubleArrayArray();
            for (int i= 0; i < ifs.getNumFaces(); i++) {
                IntArray faceIndices=indices.item(i).toIntArray();
                Arrays.fill(fni,i);
                pipeline.processPolygon(points, faceIndices,
                        faceNormals, fnia,
                        texCoords);
            }
        } else {
            for (int i= 0; i < ifs.getNumFaces(); i++) {
                IntArray faceIndices=indices.item(i).toIntArray();
                pipeline.processPolygon(points, faceIndices,
                        normals, faceIndices,
                        texCoords);
            }
        }
    }
        // Labels
        if (ifs.getEdgeAttributes(Attribute.LABELS) != null) {
          Class shaderType =  (Class) eAppearance.getAttribute(
              ShaderUtility.nameSpace(CommonAttributes.POLYGON_SHADER,"textShader"),DefaultTextShader.class);
          
          DefaultTextShader ts = (DefaultTextShader) AttributeEntityUtility.createAttributeEntity(
              shaderType, ShaderUtility.nameSpace(CommonAttributes.POLYGON_SHADER,"textShader"),
              eAppearance);
          Font font = ts.getFont();
          Color c = ts.getDiffuseColor();
          double scale = ts.getScale().doubleValue();
          double[] offset = ts.getOffset();
          int alignment = ts.getAlignment();
          
          renderLabels(scale, offset, alignment, LabelUtility.createFaceImages(ifs, font, c), ifs.getVertexAttributes(Attribute.COORDINATES).toDoubleArrayArray(), ifs.getFaceAttributes(Attribute.INDICES).toIntArrayArray());
        }
    }
    visit((IndexedLineSet)ifs);
  }

  public void visit(PointSet p) {
    if(!shaderUptodate) setupShader();
    DoubleArrayArray a = null;
    int n= p.getNumPoints();
    if(pointShader!= null) {
        a = p.getVertexAttributes(Attribute.COORDINATES).toDoubleArrayArray();
        if(a == null) return;
        pipeline.startGeometry(p);
        for (int i= 0; i < n; i++) 
            pipeline.processPoint(a, i);
        
		// Labels
      if (p.getVertexAttributes(Attribute.LABELS) != null) {
        Class shaderType =  (Class) eAppearance.getAttribute(
            ShaderUtility.nameSpace(CommonAttributes.POINT_SHADER,"textShader"),DefaultTextShader.class);
        
        DefaultTextShader ts = (DefaultTextShader) AttributeEntityUtility.createAttributeEntity(
            shaderType, ShaderUtility.nameSpace(CommonAttributes.POINT_SHADER,"textShader"),
            eAppearance);
        Font font = ts.getFont();
        Color c = ts.getDiffuseColor();
        double scale = ts.getScale().doubleValue();
        double[] offset = ts.getOffset();
        int alignment = ts.getAlignment();
        
        renderLabels(scale, offset, alignment, LabelUtility.createPointImages(p, font, c), a, null);
      }
		}
    
//    if(false) {
//        if(a == null) 
//            a = p.getVertexAttributes(Attribute.COORDINATES).toDoubleArrayArray();
//        DoubleArrayArray normals
//            =p.getVertexAttributes(Attribute.NORMALS).toDoubleArrayArray();
//        if(a == null || normals == null) 
//            return;
//        
//        double scale = .2;
//        LineShader l =pipeline.getLineShader();
//        DefaultLineShader ns =new DefaultLineShader();
//        ns.setup(eAppearance,"");
//        pipeline.setLineShader(ns);
//        for (int i= 0; i < n; i++) {
//            DoubleArray pi=a.item(i).toDoubleArray();
//            DoubleArray ni= normals.item(i).toDoubleArray();
//            double[] q = new double[3];
//            q[0] =pi.getValueAt(0) + scale * ni.getValueAt(0);
//            q[1] =pi.getValueAt(1) + scale * ni.getValueAt(1);
//            q[2] =pi.getValueAt(2) + scale * ni.getValueAt(2);
//            DoubleArray qi = new DoubleArray(q);
//            pipeline.processLine(pi,qi);
//        }
//        pipeline.setLineShader(l);
//    }
  }

  private final SceneGraphComponent labelComp = new SceneGraphComponent();
  private void renderLabels(double scale, double[] offset, int alignment, ImageData[] imgs, DoubleArrayArray vertices, IntArrayArray indices) {
    if (imgs == null) return;
    double[] storeMatrix = (double[]) this.currentTrafo.clone();
    PolygonShader storePS = this.polygonShader;
    PointShader storePtS = this.pointShader;
    LineShader storeLS = this.lineShader;
    DefaultPolygonShader labelShader = new DefaultPolygonShader();
    pipeline.setFaceShader(this.polygonShader=labelShader);
    pipeline.setPointShader(this.pointShader = null);
    pipeline.setLineShader(this.lineShader = null);
    //pipeline.setMatrix(new Matrix().getArray());
    	EffectiveAppearance storeEA = eAppearance;
    	eAppearance = EffectiveAppearance.create();
    double[] m = new double[16];
    VecMat.invert(currentTrafo,m);

    for(int i = 0, max=imgs.length; i<max;i++) {
      ImageData img = imgs[i];

//			for(int i = 0; i<labels.getLength();i++) {
//				String li = labels.getValueAt(i);
//				ImageData img = new ImageData(LabelUtility.createImageFromString(li,font,c));
    	
    	SceneGraphComponent sgc = LabelUtility.sceneGraphForLabel(labelComp,img.getWidth()*scale, img.getHeight()*scale, offset,
    			alignment, m, LabelUtility.positionFor(i, vertices, indices));
    	labelShader = new DefaultPolygonShader();
    	labelShader.texture = new SimpleTexture(img);
    	pipeline.setFaceShader(this.polygonShader=labelShader);
    	sgc.accept(this);
    }
    pipeline.setFaceShader(this.polygonShader=storePS);
    pipeline.setPointShader(this.pointShader=storePtS);
    pipeline.setLineShader(this.lineShader=storeLS);
    eAppearance = storeEA;
    pipeline.setMatrix(this.currentTrafo=storeMatrix);
  }

  
  public void visit(Sphere s) {
    if(!shaderUptodate) setupShader();
    pipeline.startGeometry(s);
    Geometries.unitSphere().apply(pipeline);
  }
  
  public void visit(Cylinder c) {
      if(!shaderUptodate) setupShader();
      pipeline.startGeometry(c);
      Geometries.cylinder().apply(pipeline);
  }

  //
  //
  //TODO: ensure somehow, that the local lights are removed 
  //      after traversing the subtree!
  //
  
  public void visit(DirectionalLight l) {
    super.visit(l);
    if(l.isGlobal()) return; //global lights are already in the environment
    float[] color= l.getColor().getRGBColorComponents(null);
    double[] direction= new double[3];
    //VecMat.transformNormal(currentTrafo.getMatrix(),0,0,1,direction);
    VecMat.transformNormal(currentTrafo, 0, 0, 1, direction);
    VecMat.normalize(direction);
    environment.addDirectionalLight(new de.jreality.soft.DirectionalLightSoft(
      color[0], color[1], color[2], l.getIntensity(), direction));
   
  }

  public void visit(PointLight l) {
      super.visit(l);
      if(l.isGlobal()) return; //global lights are already in the environment
      float[] color= l.getColor().getRGBColorComponents(null);
      double[] direction= new double[3];
      //VecMat.transformNormal(currentTrafo.getMatrix(),0,0,-1,direction);
      VecMat.transformNormal(currentTrafo, 0, 0, -1, direction);
      VecMat.normalize(direction);
      double[] src= new double[3];
      //VecMat.transform(currentTrafo.getMatrix(),0,0,0,src);
      VecMat.transform(currentTrafo, 0, 0, 0, src);
      environment.addSpotLight(new de.jreality.soft.SpotLightSoft(
          color[0], color[1], color[2], l.getIntensity(), direction,
          src, Math.PI, 0,l.getFalloffA0(), l.getFalloffA1(), l.getFalloffA2()));
    }

  
  public void visit(SpotLight l) {
    //super.visit(l);
    if(l.isGlobal()) return; //global lights are already in the environment
    float[] color= l.getColor().getRGBColorComponents(null);
    double[] direction= new double[3];
    //VecMat.transformNormal(currentTrafo.getMatrix(),0,0,-1,direction);
    VecMat.transformNormal(currentTrafo, 0, 0, -1, direction);
    VecMat.normalize(direction);
    double[] src= new double[3];
    //VecMat.transform(currentTrafo.getMatrix(),0,0,0,src);
    VecMat.transform(currentTrafo, 0, 0, 0, src);
    environment.addSpotLight(new de.jreality.soft.SpotLightSoft(
        color[0], color[1], color[2], l.getIntensity(), direction,
        src, l.getConeAngle(), l.getConeDeltaAngle(),l.getFalloffA0(), l.getFalloffA1(), l.getFalloffA2()));
  }
  
}