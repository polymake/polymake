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

package de.jreality.softviewer;

import java.awt.Color;
import java.awt.Font;

import de.jreality.backends.label.LabelUtility;
import de.jreality.backends.texture.SimpleTexture;
import de.jreality.math.Pn;
import de.jreality.scene.Appearance;
import de.jreality.scene.ClippingPlane;
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
import de.jreality.shader.DefaultGeometryShader;
import de.jreality.shader.DefaultTextShader;
import de.jreality.shader.EffectiveAppearance;
import de.jreality.shader.ImageData;
import de.jreality.shader.RenderingHintsShader;
import de.jreality.shader.ShaderUtility;
import de.jreality.softviewer.shader.DefaultPolygonShader;
import de.jreality.softviewer.shader.LineShader;
import de.jreality.softviewer.shader.PointShader;
import de.jreality.softviewer.shader.PolygonShader;
import de.jreality.util.CameraUtility;

/**
 * This class traverses a scene graph starting from the given "root" scene graph
 * component. The children of each sgc are visited in the following order: First
 * the appearance is visited then the current transformation is popped to the
 * transformationstack and a copy of it gets multiplied by the transformation of
 * the sgc. This copy is then visited. Then all geometries are visited. Finally
 * the transformation gets poped from the stack.
 * 
 * @version 1.0
 * @author <a href="mailto:hoffmann@math.tu-berlin.de">Tim Hoffmann</a>
 */
public class RenderingVisitor extends SceneGraphVisitor {
    // private static final String FACE_SHADER =
    // /*"faceShader."+*/CommonAttributes.POLYGON_SHADER;
    private static final boolean POINT_SPHERES = true;
    private static final boolean LINE_CYLINDERS = true;
    private static final Cylinder CYLINDER = new Cylinder();
    
    private boolean shaderUptodate;

    protected Environment environment = new Environment();

    protected EffectiveAppearance eAppearance;

    double[] initialTrafo, currentTrafo;

    private Transformation initialTransformation;

    protected LineShader lineShader;

    protected TrianglePipeline pipeline;

    // protected PolygonShader pointOutlineShader;
    protected PointShader pointShader;

    protected PolygonShader polygonShader;

    protected RenderingVisitor reclaimableSubcontext;
    private double levelOfDetail;
    private boolean bestQuality = false;
    private boolean transparencyEnabled = false;
    private ClippingPlaneSoft cps;
    
    public boolean isBestQuality() {
        return bestQuality;
    }

    public void setBestQuality(boolean bestQuality) {
        this.bestQuality = bestQuality;
    }

    /**
     * 
     */
    public RenderingVisitor() {
        super();
        eAppearance = EffectiveAppearance.create();
    }

    protected RenderingVisitor(RenderingVisitor parentContext) {
        eAppearance = parentContext.eAppearance;
        initializeFromParentContext(parentContext);
    }

    /**
     * @return PolygonPipeline
     */
    public TrianglePipeline getPipeline() {
        return pipeline;
    }

    /**
     * Sets the pipeline.
     * 
     * @param pipeline
     *            The pipeline to set
     */
    public void setPipeline(TrianglePipeline pipeline) {
        if (this.pipeline != null)
            this.pipeline.setEnvironment(null);
        this.pipeline = pipeline;
        pipeline.setEnvironment(environment);
    }

    protected void initializeFromParentContext(RenderingVisitor parentContext) {
        RenderingVisitor p = parentContext;
        environment = p.environment;
        eAppearance = parentContext.eAppearance;
        pipeline = p.pipeline;
        pipeline.setFaceShader(polygonShader = p.polygonShader);
        pipeline.setLineShader(lineShader = p.lineShader);
        pipeline.setPointShader(pointShader = p.pointShader);
        shaderUptodate = p.shaderUptodate;
        levelOfDetail = p.levelOfDetail;
        bestQuality = p.bestQuality;
        transparencyEnabled = p.transparencyEnabled;
        cps = null;
        // pipeline.setPointOutlineShader(pointOutlineShader=p.pointOutlineShader);
        pipeline
                .setMatrix(currentTrafo = initialTrafo = parentContext.currentTrafo);
    }

    /**
     * Sets the initialTransformation.
     * 
     * @param initialTransformation
     *            The initialTransformation to set
     */
    public void setInitialTransformation(Transformation initialTransformation) {
        this.initialTransformation = initialTransformation;
        environment.setInitialTransformation(initialTransformation);
    }

    RenderingVisitor subContext() {
        if (reclaimableSubcontext != null) {
            reclaimableSubcontext.initializeFromParentContext(this);
            // TODO is this o.k. ?
            // reclaimableSubcontext.shaderUptodate = false;
            // reclaimableSubcontext.shaderUptodate = this.shaderUptodate;
            // reclaimableSubcontext.pointShader = this.pointShader;
            // reclaimableSubcontext.lineShader = this.lineShader;
            // reclaimableSubcontext.polygonShader = this.polygonShader;
            return reclaimableSubcontext;
        } else
            return reclaimableSubcontext = new RenderingVisitor(this);
    }

    /**
     * This starts the traversal of a SceneGraph starting form root.
     * 
     * @param root
     */
    public void traverse(SceneGraphComponent root) {
        environment.removeAll();
        if (initialTrafo == null)
            initialTrafo = new double[16];
        if (initialTransformation != null)
            initialTransformation.getMatrix(initialTrafo);
        else
            VecMat.assignIdentity(initialTrafo);
        currentTrafo = initialTrafo;
        levelOfDetail = bestQuality?10000:CommonAttributes.LEVEL_OF_DETAIL_DEFAULT;
        environment.traverse(root);
        root.accept(this);
        pipeline.setMatrix(initialTrafo);
    }

    public void visit(SceneGraphComponent c) {
        if (c.isVisible())
            c.childrenAccept(subContext());
        if (cps != null) {
        	environment.removeClippingPlane(cps);
        	cps = null;
        }
    }

    public void visit(ClippingPlane cp) {
    	if (!cp.isLocal()) return;
        double[] direction= new double[3];
        //VecMat.transformNormal(currentTrafo.getMatrix(),0,0,-1,direction);
        VecMat.transformNormal(currentTrafo, 0, 0, -1, direction);
        VecMat.normalize(direction);
        double[] src= new double[3];
        //VecMat.transform(currentTrafo.getMatrix(),0,0,0,src);
        VecMat.transform(currentTrafo, 0, 0, 0, src);
        environment.addClippingPlane(cps = new ClippingPlaneSoft(direction, src));
    }
   
    public void visit(Transformation t) {
        if (initialTrafo == currentTrafo)
            currentTrafo = new double[16];
        VecMat.copyMatrix(initialTrafo, currentTrafo);
        VecMat.multiplyFromRight(currentTrafo, t.getMatrix());
        pipeline.setMatrix(currentTrafo);
    }

    public void visit(Appearance app) {
        eAppearance = eAppearance.create(app);
        shaderUptodate = false;
        if(! bestQuality) {
            Object lod =  app.getAttribute(CommonAttributes.LEVEL_OF_DETAIL, Double.class);
            if(lod instanceof Double)
                levelOfDetail = ((Double) lod).doubleValue();
        }
        Object te = app.getAttribute(CommonAttributes.TRANSPARENCY_ENABLED, Boolean.class);
        if( te instanceof Boolean) transparencyEnabled = ((Boolean) te).booleanValue();

    }

    private void setupShader() {
        pipeline.setTransparencyEnabled(transparencyEnabled);
        //levelOfDetail = eAppearance.getAttribute(CommonAttributes.LEVEL_OF_DETAIL, CommonAttributes.LEVEL_OF_DETAIL_DEFAULT);
        DefaultGeometryShader gs = ShaderUtility
                .createDefaultGeometryShader(eAppearance);
        de.jreality.shader.PolygonShader pgs;
        de.jreality.shader.LineShader lis;
        de.jreality.shader.PointShader pts;
        RenderingHintsShader rhs = ShaderUtility.createRenderingHintsShader(eAppearance);
        if (gs.getShowFaces()) {
            pgs = gs.getPolygonShader();
            // Texture2D tex = null;
            // if (AttributeEntityUtility.hasAttributeEntity(Texture2D.class,
            // ShaderUtility.nameSpace(name,"texture2d"), eAppearance)) {
            // Texture2D tex = (Texture2D)
            // AttributeEntityUtility.createAttributeEntity(Texture2D.class,
            // ShaderUtility.nameSpace(name,"texture2d"), eAppearance);
            // }
                pipeline.setFaceShader(polygonShader = PolygonShader
                        .createFrom(pgs,rhs));
                
        }
        if (gs.getShowLines()) {
            lis = gs.getLineShader();
            pipeline.setLineShader(lineShader = LineShader.createFrom(lis,rhs));
        }
        if (gs.getShowPoints()) {
            pts = gs.getPointShader();
            pipeline.setPointShader(pointShader = PointShader.createFrom(pts,rhs));
        }
        shaderUptodate = true;
    }

    public void visit(IndexedLineSet g) {
        if (!shaderUptodate)
            setupShader();
        DataList dl = g.getEdgeAttributes(Attribute.INDICES);
        if (lineShader != null && dl != null) {
            IntArrayArray edgeIndices = dl.toIntArrayArray();
            DoubleArrayArray vertices = g.getVertexAttributes(
                    Attribute.COORDINATES).toDoubleArrayArray();
            DataList radii = g.getEdgeAttributes(Attribute.RELATIVE_RADII);
            DataList colors = g.getEdgeAttributes(Attribute.COLORS);
            DoubleArray radiiArray = null;
            if(radii != null) radiiArray = radii.toDoubleArray();
            pipeline.startGeometry(g);
            PolygonShader dps = lineShader.getPolygonShader();
            for (int i = 0, n = edgeIndices.size(); i < n; i++) {
                IntArray edge = edgeIndices.item(i).toIntArray();
                double radius = lineShader.getTubeRadius();
                // world coordinates
                if(lineShader.isRadiiWorldCoordinates())
                    radius /= CameraUtility.getScalingFactor(currentTrafo, Pn.EUCLIDEAN);
                if(radiiArray != null)
                    radius *= radiiArray.getValueAt(i);
                
                for (int j = 0; j < edge.getLength() - 1; j++) {
                    DoubleArray p1 = vertices.item(edge.getValueAt(j))
                            .toDoubleArray();
                    DoubleArray p2 = vertices.item(edge.getValueAt(j + 1))
                            .toDoubleArray();
                    if(lineShader.isDrawTubes()) {
                        if(LINE_CYLINDERS) {
                        /*
                        DefaultGeometryShader gs = ShaderUtility
                        .createDefaultGeometryShader(eAppearance);
                        de.jreality.shader.LineShader ls = gs.getLineShader();
                        if(ls instanceof DefaultLineShader)
                            pApp.setAttribute("polygonShader.diffuseColor", ((DefaultLineShader)ls).getDiffuseColor());
                        if(colors!= null) {
                            DoubleArray cc = colors.item(i).toDoubleArray();
                            if(cc.size()==4)
                                pApp.setAttribute("polygonShader.transparency", cc.getValueAt(3));
                            else 
                                pApp.setAttribute("polygonShader.transparency", 0);
                            Color c = new Color((float)cc.getValueAt(0),(float) cc.getValueAt(1),(float) cc.getValueAt(1) );                        
                            pApp.setAttribute("polygonShader.diffuseColor", c);
                        }
                        EffectiveAppearance apOld = eAppearance;
                        eAppearance = eAppearance.create(pApp);
                        setupShader();
                        */
                        //PolygonShader dps = lineShader.getPolygonShader();
                        
                        double red = dps.getRed();
                        double green = dps.getGreen();
                        double blue = dps.getBlue();
                        if(colors!= null) {
                            DoubleArray  cc = colors.item(i).toDoubleArray();
                            dps.setColor(cc.getValueAt(0), cc.getValueAt(1), cc.getValueAt(2));
                        }
                        pipeline.setFaceShader(dps);
                        cylinder2(p1, p2,radius);
                        dps.setColor(red,green,blue);
                        pipeline.setFaceShader(polygonShader);
                        //eAppearance = apOld;
                        //shaderUptodate = false;
                        } else
                            pipeline.processPseudoTube(p1, p2,radius,colors!=null?colors.item(i).toDoubleArray():null);
                    } else { // lineShader.isDrawTubes == false:
                        //PolygonShader dps = lineShader.getPolygonShader();
                        if(colors!= null) {
                            DoubleArray  cc = colors.item(i).toDoubleArray();
                            dps.setColor(cc.getValueAt(0), cc.getValueAt(1), cc.getValueAt(2));
                        }
                        pipeline.processLine(p1, p2);
                    }
                }
                // int ix1=edge.getValueAt(0), ix2=edge.getValueAt(1);
                // DoubleArray p1=vertices.item(ix1).toDoubleArray();
                // DoubleArray p2=vertices.item(ix2).toDoubleArray();
                // pipeline.processLine(p1, p2);
            }
            // Labels
            if (g.getEdgeAttributes(Attribute.LABELS) != null) {
                Class shaderType = (Class) eAppearance.getAttribute(
                        ShaderUtility.nameSpace(CommonAttributes.LINE_SHADER,
                                "textShader"), DefaultTextShader.class);

                DefaultTextShader ts = (DefaultTextShader) AttributeEntityUtility
                        .createAttributeEntity(shaderType, ShaderUtility
                                .nameSpace(CommonAttributes.LINE_SHADER,
                                        "textShader"), eAppearance);
                Font font = ts.getFont();
                Color c = ts.getDiffuseColor();
                double scale = ts.getScale().doubleValue();
                double[] offset = ts.getOffset();
                int alignment = ts.getAlignment();

                renderLabels(scale, offset, alignment, LabelUtility
                        .createEdgeImages(g, font, c), g.getVertexAttributes(
                        Attribute.COORDINATES).toDoubleArrayArray(), g
                        .getEdgeAttributes(Attribute.INDICES).toIntArrayArray());
            }
        }
        visit((PointSet) g);
    }
    double[] cmat = new double[16];
    double[] cnormal = new double[3];
    double[] ctrans = new double[3];
    
    /**
     * tubes the line connecting p1 and p2
     * This deals with tubes for which both endpoints are finite.
     * @see cylinder2 for the other case
     * @param p1 first end point
     * @param p2 second endpoint
     * @param radius the radius of the tube
     */
    private void cylinder(DoubleArray p1, DoubleArray p2, double radius) {
        double w1 = (p1.size()==4)?1/p1.getValueAt(3):1;
        double w2 = (p2.size()==4)?1/p2.getValueAt(3):1;
       
        cnormal[0] = .5*(p2.getValueAt(0)*w2-p1.getValueAt(0)*w1);
        cnormal[1] = .5*(p2.getValueAt(1)*w2-p1.getValueAt(1)*w1);
        cnormal[2] = .5*(p2.getValueAt(2)*w2-p1.getValueAt(2)*w1);
        ctrans[0] = p1.getValueAt(0)*w1 + cnormal[0];
        ctrans[1] = p1.getValueAt(1)*w1 + cnormal[1];
        ctrans[2] = p1.getValueAt(2)*w1 + cnormal[2];
        double d = VecMat.norm(cnormal);
        if(d !=0) VecMat.normalize(cnormal);
        
        VecMat.normalToEuler(cnormal,0);

        VecMat.copyMatrix( currentTrafo,tmpTrafo);
        
        VecMat.assignTranslation(cmat, ctrans);
        VecMat.multiplyFromRight(tmpTrafo, cmat);

        VecMat.assignRotationZ(cmat, cnormal[2]);
        VecMat.multiplyFromRight(tmpTrafo, cmat);
        
        VecMat.assignRotationY(cmat, cnormal[1]);
        VecMat.multiplyFromRight(tmpTrafo, cmat);
        
        VecMat.assignRotationX(cmat, cnormal[0]+Math.PI/2);
        VecMat.multiplyFromRight(tmpTrafo, cmat);
        
        VecMat.assignScale(cmat, radius,radius,d);
        VecMat.multiplyFromRight(tmpTrafo, cmat);

        pipeline.setMatrix(tmpTrafo);
        visit(CYLINDER);
        pipeline.setMatrix(currentTrafo);
    }
    /**
     * tubes the line connecting p1 and p2
     * This deals with tubes fro which one of the endpoints lies at infinity
     * @see cylinder for the case of both endpoints being finite
     * @param p1 first end point
     * @param p2 second endpoint
     * @param radius the radius of the tube
     */
    private void cylinder2(DoubleArray p1, DoubleArray p2, double radius) {
        double w1 = (p1.size()==4)?p1.getValueAt(3):1;
        double w2 = (p2.size()==4)?p2.getValueAt(3):1;
        if(w1!=0 && w2!= 0) {
            cylinder(p1,p2,radius);
            return;
        }
        if(w1 == 0) {
            //if both endpoints are at infinity there is nothing to do.
            if(w2 == 0)
                return;
            w1 = w2;
            w2 = 0;
            DoubleArray tmp = p1;
            p1 = p2;
            p2 = tmp;
        }
        cnormal[0] = -(p2.getValueAt(0)*w1-p1.getValueAt(0)*w2);
        cnormal[1] = -(p2.getValueAt(1)*w1-p1.getValueAt(1)*w2);
        cnormal[2] = -(p2.getValueAt(2)*w1-p1.getValueAt(2)*w2);
        ctrans[0] = p1.getValueAt(0)/w1;
        ctrans[1] = p1.getValueAt(1)/w1;
        ctrans[2] = p1.getValueAt(2)/w1;
        
        double d = VecMat.norm(cnormal);
        if(d ==0)
            return;
        VecMat.normalize(cnormal);
        VecMat.normalToEuler(cnormal,0);

        VecMat.copyMatrix( currentTrafo,tmpTrafo);
        
        VecMat.assignTranslation(cmat, ctrans);
        VecMat.multiplyFromRight(tmpTrafo, cmat);

 //       VecMat.assignRotationZ(cmat, cnormal[2]);
 //       VecMat.multiplyFromRight(tmpTrafo, cmat);
        
        VecMat.assignRotationY(cmat, cnormal[1]);
        VecMat.multiplyFromRight(tmpTrafo, cmat);
        
        VecMat.assignRotationX(cmat, cnormal[0]+Math.PI/2);
        VecMat.multiplyFromRight(tmpTrafo, cmat);
        
        VecMat.assignScale(cmat, radius,radius,1);
        VecMat.multiplyFromRight(tmpTrafo, cmat);

        pipeline.setMatrix(tmpTrafo);
        //visit(CYLINDER);
        ////
       
       if (!shaderUptodate)
           setupShader();
       LineShader lso = lineShader;
       lineShader = null;
       PointShader pso = pointShader;
       pointShader = null;
       pipeline.startGeometry(CYLINDER);
       double l = lod(1.);
       PrimitiveCache.renderCylinder2(pipeline, l*levelOfDetail);
       pointShader = pso;
       lineShader = lso;
       
       ////
        pipeline.setMatrix(currentTrafo);
    }

 //   private int[] fni = new int[Polygon.VERTEX_LENGTH];

//    private IntArray fnia = new IntArray(fni);

    public void visit(IndexedFaceSet ifs) {
        if (!shaderUptodate)
            setupShader();

        if (polygonShader != null) {
            // vertex coordinates
            DataList indices = ifs.getFaceAttributes(Attribute.INDICES);
            if (indices != null) {
                DoubleArrayArray points = ifs.getVertexAttributes(
                        Attribute.COORDINATES).toDoubleArrayArray();
                // vertex normals
                DoubleArrayArray normals = null;
                DataList vndl = ifs.getVertexAttributes(Attribute.NORMALS);
                if (vndl != null)
                    normals = vndl.toDoubleArrayArray();
                //texture coordinates
                DataList texCoords = (ifs
                        .getVertexAttributes(Attribute.TEXTURE_COORDINATES));
                pipeline.startGeometry(ifs);

                // face normals
                DoubleArrayArray faceNormals = null;
                DataList fndl = ifs.getFaceAttributes(Attribute.NORMALS);
                if (fndl != null)
                    faceNormals = fndl.toDoubleArrayArray();

                // vertex colors
                DataList vertexColors = ifs
                        .getVertexAttributes(Attribute.COLORS);
                // faceColors
                DataList faceColors = ifs.getFaceAttributes(Attribute.COLORS);

                for (int i = 0, im = ifs.getNumFaces(); i < im; i++) {
                    IntArray faceIndices = indices.item(i).toIntArray();
                    DoubleArray faceNormal = null;
                    if (fndl != null)
                        faceNormal = faceNormals.item(i).toDoubleArray();
                    DoubleArray faceColor = null;
                    if (faceColors != null)
                        faceColor = faceColors.item(i).toDoubleArray();
                    pipeline.processPolygon(points, faceIndices, normals,
                            faceIndices, texCoords, vertexColors, faceNormal,
                            faceColor);
                }

            }
            // Labels
            if (ifs.getFaceAttributes(Attribute.LABELS) != null) {
                Class shaderType = (Class) eAppearance.getAttribute(
                        ShaderUtility.nameSpace(
                                CommonAttributes.POLYGON_SHADER, "textShader"),
                        DefaultTextShader.class);

                DefaultTextShader ts = (DefaultTextShader) AttributeEntityUtility
                        .createAttributeEntity(shaderType, ShaderUtility
                                .nameSpace(CommonAttributes.POLYGON_SHADER,
                                        "textShader"), eAppearance);
                Font font = ts.getFont();
                Color c = ts.getDiffuseColor();
                double scale = ts.getScale().doubleValue();
                double[] offset = ts.getOffset();
                int alignment = ts.getAlignment();

                renderLabels(scale, offset, alignment, LabelUtility
                        .createFaceImages(ifs, font, c), ifs
                        .getVertexAttributes(Attribute.COORDINATES)
                        .toDoubleArrayArray(), ifs.getFaceAttributes(
                        Attribute.INDICES).toIntArrayArray());
            }
        }
        visit((IndexedLineSet) ifs);
    }
    double[] pmat = new double[16];
    double[] tmpTrafo = new double[16];

    private Sphere SPHERE = new Sphere();
    Appearance pApp = new Appearance();
    public void visit(PointSet p) {
        if (!shaderUptodate)
            setupShader();
        DoubleArrayArray a = null;
        int n = p.getNumPoints();
        if (pointShader != null) {
            a = p.getVertexAttributes(Attribute.COORDINATES)
                    .toDoubleArrayArray();
            if (a == null)
                return;
            pipeline.startGeometry(p);
            DataList vertexColors = p.getVertexAttributes(Attribute.COLORS);
            DataList vertexRadii = p.getVertexAttributes(Attribute.RELATIVE_RADII);
            if(!pointShader.isSphereDraw())
                    for (int i = 0; i < n; i++)
                        pipeline.processPoint(a, i,vertexColors,vertexRadii);
            else if(!POINT_SPHERES )
                for (int i = 0; i < n; i++)
                    pipeline.processPseudoSphere(a, i,vertexColors,vertexRadii);
            else
                for (int i = 0; i < n; i++) {
                    double r = pointShader.getPointRadius();
                    if(pointShader.isRadiiWorldCoordinates())
                        r /= CameraUtility.getScalingFactor(currentTrafo, Pn.EUCLIDEAN);
                    
                    r*=(vertexRadii!=null?vertexRadii.toDoubleArray().getValueAt(i):1);
                    pmat[0]  = pmat[5]  = pmat[10] = r;
                    DoubleArray da = a.item(i).toDoubleArray();
                    pmat[3] = da.getValueAt(0);
                    pmat[7] = da.getValueAt(1);
                    pmat[11] = da.getValueAt(2);
                    pmat[15] = da.size()==4?da.getValueAt(3):1;
                    //points at infinity are not visible anyways
                    if(pmat[15]== 0)
                        return;
                    VecMat.copyMatrix( currentTrafo,tmpTrafo);
                    VecMat.multiplyFromRight(tmpTrafo, pmat);
                    pipeline.setMatrix(tmpTrafo);
                    //TODO set shader and vertexcolors correctly
                    /*
                    DefaultGeometryShader gs = ShaderUtility
                    .createDefaultGeometryShader(eAppearance);
                    de.jreality.shader.PointShader pts = gs.getPointShader();
                    if(pts instanceof DefaultPointShader)
                        pApp.setAttribute("polygonShader.diffuseColor", ((DefaultPointShader)pts).getDiffuseColor());
                    if(vertexColors!= null) {
                        DoubleArray cc = vertexColors.item(i).toDoubleArray();
                        if(cc.size()==4)
                            pApp.setAttribute("polygonShader.transparency", cc.getValueAt(3));
                        else 
                            pApp.setAttribute("polygonShader.transparency", 0);
                        Color c = new Color((float)cc.getValueAt(0),(float) cc.getValueAt(1),(float) cc.getValueAt(1) );                        
                        pApp.setAttribute("polygonShader.diffuseColor", c);
                    }
                    EffectiveAppearance apOld = eAppearance;
                    eAppearance = eAppearance.create(pApp);
                    setupShader();
                    */
                    PolygonShader dps= pointShader.getCoreShader();
                    double red = dps.getRed();
                    double g = dps.getGreen();
                    double b = dps.getBlue();
                    if(vertexColors!= null) {
                        DoubleArray  cc = vertexColors.item(i).toDoubleArray();
                        dps.setColor(cc.getValueAt(0), cc.getValueAt(1), cc.getValueAt(2));
                    }
                    pipeline.setFaceShader(dps);
                    visit(SPHERE );
                    dps.setColor(red, g, b);
                    pipeline.setFaceShader(polygonShader);
                    pipeline.setMatrix(currentTrafo);
                    //eAppearance = apOld;
                   
                    //shaderUptodate = false;
                    //pipeline.processPoint(a, i,vertexColors,vertexRadii);
                }
            // Labels
            if (p.getVertexAttributes(Attribute.LABELS) != null) {
                Class shaderType = (Class) eAppearance.getAttribute(
                        ShaderUtility.nameSpace(CommonAttributes.POINT_SHADER,
                                "textShader"), DefaultTextShader.class);

                DefaultTextShader ts = (DefaultTextShader) AttributeEntityUtility
                        .createAttributeEntity(shaderType, ShaderUtility
                                .nameSpace(CommonAttributes.POINT_SHADER,
                                        "textShader"), eAppearance);
                Font font = ts.getFont();
                Color c = ts.getDiffuseColor();
                double scale = ts.getScale().doubleValue();
                double[] offset = ts.getOffset();
                int alignment = ts.getAlignment();

                renderLabels(scale, offset, alignment, LabelUtility
                        .createPointImages(p, font, c), a, null);
            }
        }

        // if(false) {
        // if(a == null)
        // a =
        // p.getVertexAttributes(Attribute.COORDINATES).toDoubleArrayArray();
        // DoubleArrayArray normals
        // =p.getVertexAttributes(Attribute.NORMALS).toDoubleArrayArray();
        // if(a == null || normals == null)
        // return;
        //        
        // double scale = .2;
        // LineShader l =pipeline.getLineShader();
        // DefaultLineShader ns =new DefaultLineShader();
        // ns.setup(eAppearance,"");
        // pipeline.setLineShader(ns);
        // for (int i= 0; i < n; i++) {
        // DoubleArray pi=a.item(i).toDoubleArray();
        // DoubleArray ni= normals.item(i).toDoubleArray();
        // double[] q = new double[3];
        // q[0] =pi.getValueAt(0) + scale * ni.getValueAt(0);
        // q[1] =pi.getValueAt(1) + scale * ni.getValueAt(1);
        // q[2] =pi.getValueAt(2) + scale * ni.getValueAt(2);
        // DoubleArray qi = new DoubleArray(q);
        // pipeline.processLine(pi,qi);
        // }
        // pipeline.setLineShader(l);
        // }
    }
    double[] p1 = new double[8];
    double[] p2 = new double[8];
    private double lod(double r) {
        p1[0] = p1[1] = p1[2] = p2[1] = p2[2] = 0;
        p2[0] = r;
        p1[3] = p2[3] = 1;
        pipeline.transformNDC(p1,p2);
        p1[0] = p1[4];
        p1[1] = p1[5];
        p1[2] = p1[6];
        VecMat.multiply(p1, 1/p1[7]);
        p2[0] = p2[4];
        p2[1] = p2[5];
        p2[2] = p2[6];
        VecMat.multiply(p2, 1/p2[7]);
        VecMat.vecAssignMinus(p2, p1);
        return VecMat.norm(p2);
    }
    private final SceneGraphComponent labelComp = new SceneGraphComponent();

    private void renderLabels(double scale, double[] offset, int alignment,
            ImageData[] imgs, DoubleArrayArray vertices, IntArrayArray indices) {
        if (imgs == null)
            return;
        double[] storeMatrix = (double[]) this.currentTrafo.clone();
        PolygonShader storePS = this.polygonShader;
        PointShader storePtS = this.pointShader;
        LineShader storeLS = this.lineShader;
        //DefaultPolygonShader labelShader = new DefaultPolygonShader();
        //pipeline.setFaceShader(this.polygonShader = labelShader);
        pipeline.setPointShader(this.pointShader = null);
        pipeline.setLineShader(this.lineShader = null);
        // pipeline.setMatrix(new Matrix().getArray());
        EffectiveAppearance storeEA = eAppearance;
        eAppearance = EffectiveAppearance.create();
        double[] m = new double[16];
        VecMat.invert(currentTrafo, m);

        for (int i = 0, max = imgs.length; i < max; i++) {
            ImageData img = imgs[i];

            // for(int i = 0; i<labels.getLength();i++) {
            // String li = labels.getValueAt(i);
            // ImageData img = new
            // ImageData(LabelUtility.createImageFromString(li,font,c));

            SceneGraphComponent sgc = LabelUtility.sceneGraphForLabel(
                    labelComp, img.getWidth() * scale, img.getHeight() * scale,
                    offset, alignment, m, LabelUtility.positionFor(i, vertices,
                            indices));
            DefaultPolygonShader labelShader = new DefaultPolygonShader();
            labelShader.setTexture(new SimpleTexture(img));
            pipeline.setFaceShader(this.polygonShader = labelShader);
            sgc.accept(this);
        }
        pipeline.setFaceShader(this.polygonShader = storePS);
        pipeline.setPointShader(this.pointShader = storePtS);
        pipeline.setLineShader(this.lineShader = storeLS);
        eAppearance = storeEA;
        pipeline.setMatrix(this.currentTrafo = storeMatrix);
    }

    public void visit(Sphere s) {
        if (!shaderUptodate)
            setupShader();
        LineShader lso = lineShader;
        lineShader = null;
        PointShader pso = pointShader;
        pointShader = null;
        pipeline.startGeometry(s);
        double l = lod(1.);
        //PrimitiveCache.getSphere(l).accept(this);
        PrimitiveCache.renderSphere(pipeline, l*levelOfDetail);
        pointShader = pso;
        lineShader = lso;
    }

    public void visit(Cylinder c) {
        if (!shaderUptodate)
            setupShader();
        LineShader lso = lineShader;
        lineShader = null;
        PointShader pso = pointShader;
        pointShader = null;
        pipeline.startGeometry(c);
        double l = lod(1.);
        PrimitiveCache.renderCylinder(pipeline, l*levelOfDetail);
        //PrimitiveCache.getCylinder().accept(this);
        pointShader = pso;
        lineShader = lso;
    }

    //
    //
    // TODO: ensure somehow, that the local lights are removed
    // after traversing the subtree!
    //

    public void visit(DirectionalLight l) {
        super.visit(l);
        if (l.isGlobal())
            return; // global lights are already in the environment
        float[] color = l.getColor().getRGBColorComponents(null);
        double[] direction = new double[3];
        // VecMat.transformNormal(currentTrafo.getMatrix(),0,0,1,direction);
        VecMat.transformNormal(currentTrafo, 0, 0, 1, direction);
        VecMat.normalize(direction);
        environment
                .addDirectionalLight(new de.jreality.softviewer.DirectionalLightSoft(
                        color[0], color[1], color[2], l.getIntensity(),
                        direction));

    }

    public void visit(PointLight l) {
        super.visit(l);
        if (l.isGlobal())
            return; // global lights are already in the environment
        float[] color = l.getColor().getRGBColorComponents(null);
        double[] direction = new double[3];
        // VecMat.transformNormal(currentTrafo.getMatrix(),0,0,-1,direction);
        VecMat.transformNormal(currentTrafo, 0, 0, -1, direction);
        VecMat.normalize(direction);
        double[] src = new double[3];
        // VecMat.transform(currentTrafo.getMatrix(),0,0,0,src);
        VecMat.transform(currentTrafo, 0, 0, 0, src);
        environment.addSpotLight(new de.jreality.softviewer.SpotLightSoft(color[0],
                color[1], color[2], l.getIntensity(), direction, src, Math.PI,
                0, l.getFalloffA0(), l.getFalloffA1(), l.getFalloffA2()));
    }

    public void visit(SpotLight l) {
        // super.visit(l);
        if (l.isGlobal())
            return; // global lights are already in the environment
        float[] color = l.getColor().getRGBColorComponents(null);
        double[] direction = new double[3];
        // VecMat.transformNormal(currentTrafo.getMatrix(),0,0,-1,direction);
        VecMat.transformNormal(currentTrafo, 0, 0, -1, direction);
        VecMat.normalize(direction);
        double[] src = new double[3];
        // VecMat.transform(currentTrafo.getMatrix(),0,0,0,src);
        VecMat.transform(currentTrafo, 0, 0, 0, src);
        environment.addSpotLight(new de.jreality.softviewer.SpotLightSoft(color[0],
                color[1], color[2], l.getIntensity(), direction, src, l
                        .getConeAngle(), l.getConeDeltaAngle(), l
                        .getFalloffA0(), l.getFalloffA1(), l.getFalloffA2()));
    }

}