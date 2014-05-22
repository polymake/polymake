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

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;
import java.util.logging.Logger;

import de.jreality.math.Pn;
import de.jreality.math.Rn;
import de.jreality.scene.Appearance;
import de.jreality.scene.Cylinder;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.IndexedLineSet;
import de.jreality.scene.PointSet;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SceneGraphPath;
import de.jreality.scene.SceneGraphVisitor;
import de.jreality.scene.Sphere;
import de.jreality.scene.Transformation;
import de.jreality.scene.data.Attribute;
import de.jreality.scene.data.DataList;
import de.jreality.scene.data.DoubleArray;
import de.jreality.scene.data.DoubleArrayArray;
import de.jreality.scene.data.IntArray;
import de.jreality.scene.data.IntArrayArray;
import de.jreality.scene.pick.PickResult;
import de.jreality.shader.CommonAttributes;
import de.jreality.shader.EffectiveAppearance;
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
public class PickVisitor extends SceneGraphVisitor {
    private static final String FACE_SHADER = /*"faceShader."+*/CommonAttributes.POLYGON_SHADER;
  private boolean shaderUptodate;

  protected Environment     environment = new Environment();
  protected HitDetector hitDetector = new HitDetector();
  protected EffectiveAppearance eAppearance;

  double[]  initialTrafo,   currentTrafo;
  private   Transformation  initialTransformation;
  protected LineShader   lineShader = new DefaultLineShader(new DefaultPolygonShader(new ConstantVertexShader()));
  protected PolygonPipeline pipeline;
  protected PointShader   pointShader = new DefaultPointShader(new DefaultPolygonShader(new ConstantVertexShader()),new DefaultPolygonShader(new ConstantVertexShader()));
  protected PolygonShader   polygonShader = new DefaultPolygonShader(new ConstantVertexShader());

  protected PickVisitor reclaimableSubcontext;

  /**
   * 
   */
  public PickVisitor() {
    super();
    eAppearance=EffectiveAppearance.create();
    
  }

  protected PickVisitor(PickVisitor parentContext) {
    eAppearance=parentContext.eAppearance;
    initializeFromParentContext(parentContext);
  }

  private void hit() {
      System.out.println("hit");
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
  protected void initializeFromParentContext(PickVisitor parentContext) {
    PickVisitor p=parentContext;
    environment=p.environment;
    eAppearance=parentContext.eAppearance;
    hitDetector = parentContext.hitDetector;
    pipeline=p.pipeline;
    pipeline.setFaceShader(polygonShader=p.polygonShader);
    pipeline.setLineShader(lineShader=p.lineShader);
    pipeline.setPointShader(pointShader=p.pointShader);
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

  PickVisitor subContext() {
    if (reclaimableSubcontext != null) {
      reclaimableSubcontext.initializeFromParentContext(this);
      reclaimableSubcontext.shaderUptodate = false;
      return reclaimableSubcontext;
    } else
      return reclaimableSubcontext= new PickVisitor(this);
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
    // we do not want lights since wo don't render anyways
//    environment.traverse(root);
    hitDetector.clear();
    double[] im = new double[16];
    Rn.inverse(im,initialTrafo);
    hitDetector.setCameraToWorld(im);
    visit(root);
    pipeline.setMatrix(initialTrafo);
  }

  public void visit(SceneGraphComponent c) {
    if(c.isVisible()) {
        hitDetector.path.push(c);
        c.childrenAccept(subContext());
        hitDetector.path.pop();
    }
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
    
    if((boolean)eAppearance.getAttribute(ShaderUtility.nameSpace(geomShaderName, CommonAttributes.FACE_DRAW), true)) {
        PolygonShader fs=ShaderLookup
            .getPolygonShaderAttr(eAppearance, geomShaderName, FACE_SHADER);
        pipeline.setFaceShader(this.polygonShader=fs);
    } else {
        pipeline.setFaceShader(this.polygonShader=null);
    }
    if((boolean)eAppearance.getAttribute(ShaderUtility.nameSpace(geomShaderName, CommonAttributes.EDGE_DRAW), true)) {
        LineShader ls=ShaderLookup
       .getLineShaderAttr(eAppearance, geomShaderName, CommonAttributes.LINE_SHADER);;
       pipeline.setLineShader(this.lineShader=ls);
    } else {
        pipeline.setLineShader(this.lineShader=null);
    }
       
    if((boolean)eAppearance.getAttribute(ShaderUtility.nameSpace(geomShaderName, CommonAttributes.VERTEX_DRAW), true)) {
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
      hitDetector.path.push(g);

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
    }
      hitDetector.path.pop();
    visit((PointSet)g);
  }
  
  private int[] fni =new int[Polygon.VERTEX_LENGTH];
  private IntArray fnia =new IntArray(fni);
  
  public void visit(IndexedFaceSet ifs) {
      hitDetector.path.push(ifs);

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
    }
      hitDetector.path.pop();
    visit((IndexedLineSet)ifs);
  }

  public void visit(PointSet p) {
      hitDetector.path.push(p);
    if(!shaderUptodate) setupShader();
    DoubleArrayArray a = null;
    int n= p.getNumPoints();
    if(pointShader!= null) {
        a = p.getVertexAttributes(Attribute.COORDINATES).toDoubleArrayArray();
        if(a == null) return;
        pipeline.startGeometry(p);
        for (int i= 0; i < n; i++) 
            pipeline.processPoint(a, i);
        
    }
      hitDetector.path.pop();
    
  }

  
  public void visit(Sphere s) {
      hitDetector.path.push(s);
    if(!shaderUptodate) setupShader();
    pipeline.startGeometry(s);
    Geometries.unitSphere().apply(pipeline);
      hitDetector.path.pop();
  }
  
  public void visit(Cylinder c) {
      hitDetector.path.push(c);
    if(!shaderUptodate) setupShader();
      pipeline.startGeometry(c);
      Geometries.cylinder().apply(pipeline);
      hitDetector.path.pop();
  }

  //
  //
  //TODO: ensure somehow, that the local lights are removed 
  //      after traversing the subtree!
  //
  
//  public void visit(DirectionalLight l) {
//    super.visit(l);
//    if(l.isGlobal()) return; //global lights are already in the environment
//    float[] color= l.getColorAsFloat();
//    double[] direction= new double[3];
//    //VecMat.transformNormal(currentTrafo.getMatrix(),0,0,1,direction);
//    VecMat.transformNormal(currentTrafo, 0, 0, 1, direction);
//    VecMat.normalize(direction);
//    environment.addDirectionalLight(new de.jreality.soft.DirectionalLightSoft(
//      color[0], color[1], color[2], l.getIntensity(), direction));
//   
//  }

//  public void visit(PointLight l) {
//      super.visit(l);
//      if(l.isGlobal()) return; //global lights are already in the environment
//      float[] color= l.getColorAsFloat();
//      double[] direction= new double[3];
//      //VecMat.transformNormal(currentTrafo.getMatrix(),0,0,-1,direction);
//      VecMat.transformNormal(currentTrafo, 0, 0, -1, direction);
//      VecMat.normalize(direction);
//      double[] src= new double[3];
//      //VecMat.transform(currentTrafo.getMatrix(),0,0,0,src);
//      VecMat.transform(currentTrafo, 0, 0, 0, src);
//      environment.addSpotLight(new de.jreality.soft.SpotLightSoft(
//          color[0], color[1], color[2], l.getIntensity(), direction,
//          src, Math.PI, 0,l.getFalloffA0(), l.getFalloffA1(), l.getFalloffA2()));
//    }

  
//  public void visit(SpotLight l) {
//    //super.visit(l);
//    if(l.isGlobal()) return; //global lights are already in the environment
//    float[] color= l.getColorAsFloat();
//    double[] direction= new double[3];
//    //VecMat.transformNormal(currentTrafo.getMatrix(),0,0,-1,direction);
//    VecMat.transformNormal(currentTrafo, 0, 0, -1, direction);
//    VecMat.normalize(direction);
//    double[] src= new double[3];
//    //VecMat.transform(currentTrafo.getMatrix(),0,0,0,src);
//    VecMat.transform(currentTrafo, 0, 0, 0, src);
//    environment.addSpotLight(new de.jreality.soft.SpotLightSoft(
//        color[0], color[1], color[2], l.getIntensity(), direction,
//        src, l.getConeAngle(), l.getConeDeltaAngle(),l.getFalloffA0(), l.getFalloffA1(), l.getFalloffA2()));
//  }
  public static class HitDetector extends NewDoublePolygonRasterizer.Colorizer {
      private Comparator comparator = new HitComparator();
      protected SceneGraphPath path = new SceneGraphPath();
      protected ArrayList list = new ArrayList();
      private double[] ndcToCamera;
      private double[] cameraToWorld;
    public double[] getCameraToWorld() {
        return cameraToWorld;
    }
    public void setCameraToWorld(double[] cameraToWorld) {
        this.cameraToWorld = cameraToWorld;
    }
    public double[] getNdcToCamera() {
        return ndcToCamera;
    }
    public void setNdcToCamera(double[] ndcToCamera) {
        this.ndcToCamera = ndcToCamera;
    }
    /* (non-Javadoc)
     * @see de.jreality.soft.NewDoublePolygonRasterizer.Colorizer#colorize(int[], double[], int, double[])
     */
    public void colorize(int[] pixels, double[] zBuff, int pos, double[] data,boolean interpolateUV) {
        //System.out.println(" current Geometry is "+path.getLastElement());
        double[] pointNDC = new double[] {0,0,data[2],1};
        double[] pointCamera = new double[4];
        Rn.matrixTimesVector(pointCamera,ndcToCamera,pointNDC);
        Pn.dehomogenize(pointCamera,pointCamera);
        double[] pointWorld = new double[4];
        Rn.matrixTimesVector(pointWorld,cameraToWorld,pointCamera);
        Pn.dehomogenize(pointWorld,pointWorld);
        
        double[] pointObject = new double[4];
        
        Rn.matrixTimesVector(pointObject,path.getInverseMatrix(null),pointWorld);
        Pn.dehomogenize(pointObject,pointObject);
        double[] texCoords =null;
        if(interpolateUV) {
            texCoords = new double[] {data[Polygon.U],data[Polygon.V]};
        }
            
        list.add(new Hit((SceneGraphPath)path.clone(),pointNDC, pointCamera, pointWorld,pointObject,texCoords));
    }
    protected void clear() {
        list = new ArrayList();
        path = new SceneGraphPath();
    }
    public List getHitList() {
        Collections.sort(list,comparator);
//        if(list.size()!= 1) {
//            System.out.println("hit length "+list.size());
//            for (Iterator iter = list.iterator(); iter.hasNext();) {
//                Hit element = (Hit) iter.next();
//                System.out.println(element.getPath().getLastElement());
//            }
//        }
        return list;
    }
  }
  
  public static class Hit implements PickResult {
      SceneGraphPath path;
      double[] pointCamera;
      double[] pointWorld;
      double[] pointNDC;
      double[] pointObject;
      double[] textureCoords;
      public Hit(SceneGraphPath path, double[] pointNDC, double[] pointCamera, double[] pointWorld, double[] pointObject,double[] textureCoords) {
          this.path = (SceneGraphPath) path;
          this.pointCamera = (double[]) pointCamera;
          this.pointWorld= (double[]) pointWorld;
          this.pointNDC= (double[]) pointNDC;
          this.pointObject= (double[]) pointObject;
          this.textureCoords = textureCoords;
      }
    public SceneGraphPath getPickPath() {
        return path;
    }
    public double[] getPointCamera() {
        return pointCamera;
    }
    public double[] getPointNDC() {
        return pointNDC;
    }
    
    public double[] getWorldCoordinates() {
        return pointWorld;
    }
    /* (non-Javadoc)
     * @see de.jreality.scene.tool.PickResult#getObjectCoordinates()
     */
    public double[] getObjectCoordinates() {
        return pointObject;
    }
    public String toString() {
      StringBuffer sb = new StringBuffer();
      sb.append("Soft-Pick: ");
      sb.append(" dist=").append(Rn.euclideanDistance(pointCamera, pointWorld));
      sb.append(" world=").append(Rn.toString(pointWorld));
      sb.append(" path=").append(path.toString());
      return sb.toString();
    }
    public int getIndex() {
      // TODO Auto-generated method stub
      return -1;
    }
    public int getPickType() {
      // TODO Auto-generated method stub
      return -1;
    }
    public int hasTextureCoordinates() {
        if(textureCoords== null)
            return 0;
        else
            return textureCoords.length;
    }
    public double[] getTextureCoordinates() {
        return textureCoords;
    }

    public int getSecondaryIndex() {
		return -1;
	}
  }
      public static class HitComparator implements Comparator {

        /* (non-Javadoc)
         * @see java.util.Comparator#compare(java.lang.Object, java.lang.Object)
         */
        public int compare(Object o1, Object o2) {
            double a = ((Hit) o1).getPointNDC()[2];
            double b = ((Hit) o2).getPointNDC()[2];
            return a>b? 1 : (b>a? -1:0);    
//            double a = ((Hit) o1).getPointCamera()[2];
//            double b = ((Hit) o2).getPointCamera()[2];
//            return a>b? 1 : (b>a? -1:0);    
        }
          
      }
  /**
   * @return
   */
  public HitDetector getHitDetector() {
      return hitDetector;
  }
  
}