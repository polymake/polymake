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

import de.jreality.scene.ClippingPlane;
import de.jreality.scene.DirectionalLight;
import de.jreality.scene.PointLight;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SceneGraphVisitor;
import de.jreality.scene.SpotLight;
import de.jreality.scene.Transformation;

/**
 * This class holds information about the environment---mainly lights and the camera at the moment.
 * TODO: Fix the treatment of the normal direcitons (needs transposeInverse matrix) 
 * @version 1.0
 * @author <a href="mailto:hoffmann@math.tu-berlin.de">Tim Hoffmann</a>
 *
 */
public final class Environment extends SceneGraphVisitor {


    //protected EffectiveAppearance eAppearance;

    double[]  initialTrafo,   currentTrafo;
    private   Transformation  initialTransformation;
    protected Environment reclaimableSubcontext;

    
    //private Camera camera;
    private double[] matrix;
    //private double[] inverseCameraMatrix= new double[16];

    private Globals globals; 

    /**
     * 
     */
    public Environment() {
        super();
        globals = new Globals();
        //eAppearance=EffectiveAppearance.create();
    }

    protected Environment(Environment parentContext) {
        //eAppearance=parentContext.eAppearance;
        initializeFromParentContext(parentContext);
    }

    
    public final DirectionalLightSoft[] getDirectionalLights() {
        return globals.getDirectionalLights();
    }
    
    
    public final void addDirectionalLight(DirectionalLightSoft l) {
        globals.addDirectionalLight(l);

    }

    public final void removeDirectionalLight(DirectionalLightSoft l) {
        globals.removeDirectionalLight(l);
    }

    /**
     * @return int
     */
    public final int getNumDirectionalLights() {
        return globals.getNumDirectionalLights();
    }
    
    public final SpotLightSoft[] getSpotLights() {
        return globals.getSpotLights();
    }

    
    public final void addSpotLight(SpotLightSoft l) {
        globals.addSpotLight(l);

    }

    public final void removeSpotLight(SpotLightSoft l) {
        globals.removeSpotLight(l);
    }

    /**
     * @return int
     */
    public final int getNumSpotLights() {
        return globals.getNumSpotLights();
    }

    public final ClippingPlaneSoft[] getClippingPlanes() {
        return globals.getClippingPlanes();
    }

    
    public final void addClippingPlane(ClippingPlaneSoft c) {
        globals.addClippingPlane(c);
    }

    public final void removeClippingPlane(ClippingPlaneSoft c) {
        globals.removeClippingPlane(c);
    }

    /**
     * @return int
     */
    public final int getNumClippingPlanes() {
        return globals.getNumClippingPlanes();
    }
    
    
    public final void removeAll() {
        globals.removeAll();
    }
    
    
    
    //
    // traversal stuff
    //
    protected void initializeFromParentContext(Environment parentContext) {
        Environment p=parentContext;
        globals = parentContext.globals;
        
        currentTrafo=initialTrafo=parentContext.currentTrafo;
    }

    /**
     * Sets the initialTransformation.
     * @param initialTransformation The initialTransformation to set
     */
    public void setInitialTransformation(Transformation initialTransformation) {
        this.initialTransformation= initialTransformation;
    }

    Environment subContext() {
        if (reclaimableSubcontext != null) {
            reclaimableSubcontext.initializeFromParentContext(this);
            return reclaimableSubcontext;
        } else
            return reclaimableSubcontext= new Environment(this);
    }
    /**
     * This starts the traversal of a SceneGraph starting form root.
     * @param root
     */
    public void traverse(SceneGraphComponent root) {
        removeAll();
        if (initialTrafo == null)
            initialTrafo= new double[16];
        if (initialTransformation != null)
            initialTransformation.getMatrix(initialTrafo);
        else
            VecMat.assignIdentity(initialTrafo);
        currentTrafo= initialTrafo;
        
        visit(root);
    }

    public void visit(SceneGraphComponent c) {
        c.childrenAccept(subContext());
    }

    public void visit(Transformation t) {
        if (initialTrafo == currentTrafo)
            currentTrafo= new double[16];
        VecMat.copyMatrix(initialTrafo, currentTrafo);
        VecMat.multiplyFromRight(currentTrafo, t.getMatrix());
    }
    
    public void visit(DirectionalLight l) {
        super.visit(l);
        if(!l.isGlobal()) return;// local lights are added at the render traversal
        float[] color= l.getColor().getRGBColorComponents(null);
        double[] direction= new double[3];
        //VecMat.transformNormal(currentTrafo.getMatrix(),0,0,1,direction);
        VecMat.transformNormal(currentTrafo, 0, 0, 1, direction);
        VecMat.normalize(direction);
        addDirectionalLight(new DirectionalLightSoft(
                color[0], color[1], color[2], l.getIntensity(), direction));
    }

    public void visit(PointLight l) {
        super.visit(l);
        if(!l.isGlobal()) return; // local lights are added at the render traversal
        float[] color= l.getColor().getRGBColorComponents(null);
        double[] direction= new double[3];
        //VecMat.transformNormal(currentTrafo.getMatrix(),0,0,-1,direction);
        VecMat.transformNormal(currentTrafo, 0, 0, 1, direction);
        VecMat.normalize(direction);
        double[] src= new double[3];
        //VecMat.transform(currentTrafo.getMatrix(),0,0,0,src);
        VecMat.transform(currentTrafo, 0, 0, 0, src);
        addSpotLight(new SpotLightSoft(
                color[0], color[1], color[2], l.getIntensity(), direction,
                src,Math.PI,0, l.getFalloffA0(), l.getFalloffA1(), l.getFalloffA2()));

    }

    
    public void visit(SpotLight l) {
        //super.visit(l);
        if(!l.isGlobal()) return; // local lights are added at the render traversal
        float[] color= l.getColor().getRGBColorComponents(null);
        double[] direction= new double[3];
        VecMat.transformNormal(currentTrafo, 0, 0, -1, direction);
        //VecMat.transformNormal(currentTrafo, 0, 0, 1, direction);
        VecMat.normalize(direction);
        double[] src= new double[3];
        //VecMat.transform(currentTrafo.getMatrix(),0,0,0,src);
        VecMat.transform(currentTrafo, 0, 0, 0, src);
        addSpotLight(new SpotLightSoft(
                color[0], color[1], color[2], l.getIntensity(), direction,
                src,l.getConeAngle(),l.getConeDeltaAngle(), l.getFalloffA0(), l.getFalloffA1(), l.getFalloffA2()));

    }
    
    public void visit(ClippingPlane c) {
        
        super.visit(c);

        double[] direction= new double[3];
        //VecMat.transformNormal(currentTrafo.getMatrix(),0,0,-1,direction);
        VecMat.transformNormal(currentTrafo, 0, 0, -1, direction);
        VecMat.normalize(direction);
        double[] src= new double[3];
        //VecMat.transform(currentTrafo.getMatrix(),0,0,0,src);
        VecMat.transform(currentTrafo, 0, 0, 0, src);
        addClippingPlane(new ClippingPlaneSoft(direction, src));
    }
    
    /**
     * A shader can ask for the current world to camera matrix using this method.
     * @return Returns the matrix.
     */
    public final double[] getMatrix() {
        return matrix;
    }

    /**
     * @param matrix The matrix to set.
     */
    public final void setMatrix(final double[] matrix) {
        this.matrix = matrix;
    }
    
    private class Globals{
        
        private SpotLightSoft[] spotLights= new SpotLightSoft[0];

        private int numSpotLights;
        
        private ClippingPlaneSoft[] clippingPlanes= new ClippingPlaneSoft[0];

        private int numClippingPlanes;

        private int numDirectionalLights;

        private DirectionalLightSoft[] directionalLights= new DirectionalLightSoft[0];
        
        
        public Globals() {
            super();
        }
        
        
        public final DirectionalLightSoft[] getDirectionalLights() {
            return directionalLights;
        }
        
        
        public final void addDirectionalLight(DirectionalLightSoft l) {
            if (l == null)
                return;
            if (numDirectionalLights == 0
                    || numDirectionalLights == directionalLights.length) {
                DirectionalLightSoft[] nl= new DirectionalLightSoft[numDirectionalLights + 5];
                if (numDirectionalLights != 0)
                    System.arraycopy(directionalLights, 0, nl, 0, numDirectionalLights);
                directionalLights= nl;
            }
            directionalLights[numDirectionalLights++]= l;

        }

        public final void removeDirectionalLight(DirectionalLightSoft l) {
            boolean on= false;
            for (int i= 0; i < numDirectionalLights - 1; i++) {
                if (!on && directionalLights[i] == l)
                    on= true;
                if (on)
                    directionalLights[i]= directionalLights[i + 1];
            }
            if (on) {
                directionalLights[numDirectionalLights - 1]= null;
                numDirectionalLights--;
            }
        }

        /**
         * @return int
         */
        public final int getNumDirectionalLights() {
            return numDirectionalLights;
        }
        
        public final SpotLightSoft[] getSpotLights() {
            return spotLights;
        }

        
        public final void addSpotLight(SpotLightSoft l) {
            if (l == null)
                return;
            if (numSpotLights == 0 || numSpotLights == spotLights.length) {
                SpotLightSoft[] nl= new SpotLightSoft[numSpotLights + 5];
                if (numSpotLights != 0)
                    System.arraycopy(spotLights, 0, nl, 0, numSpotLights);
                spotLights= nl;
            }
            spotLights[numSpotLights++]= l;

        }

        public final void removeSpotLight(SpotLightSoft l) {
            boolean on= false;
            for (int i= 0; i < numSpotLights - 1; i++) {
                if (!on && spotLights[i] == l)
                    on= true;
                if (on)
                    spotLights[i]= spotLights[i + 1];
            }
            if (on) {
                spotLights[numSpotLights - 1]= null;
                numSpotLights--;
            }
        }

        /**
         * @return int
         */
        public final int getNumSpotLights() {
            return numSpotLights;
        }

        public final ClippingPlaneSoft[] getClippingPlanes() {
            return clippingPlanes;
        }

        
        public final void addClippingPlane(ClippingPlaneSoft c) {
            if (c == null)
                return;
            if (numClippingPlanes == 0 || numClippingPlanes == clippingPlanes.length) {
                ClippingPlaneSoft[] nl= new ClippingPlaneSoft[numClippingPlanes + 5];
                if (numClippingPlanes != 0)
                    System.arraycopy(clippingPlanes, 0, nl, 0, numClippingPlanes);
                clippingPlanes= nl;
            }
            clippingPlanes[numClippingPlanes++]= c;

        }

        public final void removeClippingPlane(ClippingPlaneSoft c) {
            boolean on= false;
            for (int i= 0; i < numClippingPlanes - 1; i++) {
                if (!on && clippingPlanes[i] == c)
                    on= true;
                if (on)
                    clippingPlanes[i]= clippingPlanes[i + 1];
            }
            if (on) {
                clippingPlanes[numClippingPlanes - 1]= null;
                numClippingPlanes--;
            }
        }

        /**
         * @return int
         */
        public final int getNumClippingPlanes() {
            return numClippingPlanes;
        }
        
        
        public final void removeAll() {
            numSpotLights        = 0;
            numDirectionalLights = 0;
            numClippingPlanes    = 0;

            for (int i= 0; i < spotLights.length; i++) {
                spotLights[i]= null;
            }
            for (int i= 0; i < directionalLights.length; i++) {
                directionalLights[i]= null;
            }
            for (int i= 0; i < clippingPlanes.length; i++) {
                clippingPlanes[i]= null;
            }
        }
        
        
    }
}
