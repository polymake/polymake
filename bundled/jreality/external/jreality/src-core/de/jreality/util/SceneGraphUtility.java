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


package de.jreality.util;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.logging.Level;

import de.jreality.geometry.GeometryUtility;
import de.jreality.geometry.IndexedFaceSetUtility;
import de.jreality.math.Matrix;
import de.jreality.math.MatrixBuilder;
import de.jreality.math.P3;
import de.jreality.math.Pn;
import de.jreality.math.Rn;
import de.jreality.scene.Appearance;
import de.jreality.scene.Camera;
import de.jreality.scene.Geometry;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.Light;
import de.jreality.scene.PointSet;
import de.jreality.scene.Scene;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SceneGraphNode;
import de.jreality.scene.SceneGraphPath;
import de.jreality.scene.SceneGraphVisitor;
import de.jreality.scene.Sphere;
import de.jreality.scene.SpotLight;
import de.jreality.scene.Transformation;
import de.jreality.scene.Viewer;
import de.jreality.scene.data.Attribute;
import de.jreality.scene.data.StorageModel;
import de.jreality.shader.CommonAttributes;
import de.jreality.shader.EffectiveAppearance;


/**
 * This class holds static methods that make the parsing/traversal etc of a scene graph more comfortable.
 * @author <a href="mailto:hoffmann@math.tu-berlin.de">Tim Hoffmann</a>
 *
 */
public class SceneGraphUtility {

	public static SceneGraphComponent createFullSceneGraphComponent()	{
		return createFullSceneGraphComponent("unnamed");
	}
	
	/**
	 * Allocate and return an instance of {@link SceneGraphComponent} fitted out with an instance
	 * of {@link Transformation} and {@link Appearance}.
	 * @author Charles Gunn
	 * @param name
	 * @return
	 */public static SceneGraphComponent createFullSceneGraphComponent(String name)	{
		SceneGraphComponent c = new SceneGraphComponent();
		c.setTransformation(new Transformation());
		c.setAppearance(new Appearance());
		c.setName(name);
		return c;
	}
    /**
    * Replace the first child with the given component.
    * @author Charles Gunn
      * @param c
     * @param ch
     */
	 public static void replaceChild(SceneGraphComponent c, SceneGraphComponent ch)  {
      int n = c.getChildComponentCount();
      if (n == 0) { c.addChild(ch); return; } 
      SceneGraphComponent och = c.getChildComponent(0);
      if (och == ch) return;
      c.removeChild(och);
      c.addChild(ch);
  }
	
  /**
   * Remove all children (i.e., instances of {@link SceneGraphComponent} from this node.
   * @param component
   * 
   * TODO: this should be called removeChildComponents!
   */
  	public static void removeChildren(final SceneGraphComponent c) {
  		Scene.executeWriter(c, new Runnable()	{
			public void run() {
			  	while (c.getChildComponentCount() > 0) c.removeChild(c.getChildComponent(0));
			}
 			
  		});
  	}
	
  	/**
  	 * Set the metric of this sub-graph by setting the appearance attribute
  	 * {@link CommonAttributes.METRIC}.
  	 * @param r
  	 * @param metric
  	 */public static void setMetric(SceneGraphComponent r, int metric)	{
 		final int sig = metric;
 		 if (r.getAppearance() == null) r.setAppearance(new Appearance());
 		 r.getAppearance().setAttribute(CommonAttributes.METRIC,sig);
    	}
  
     
 	/**
 	 * Return the metric metric at the end of the path <i>sgp</i> by evaluating
 	 * effective appearance for the attribute {@link CommonAttributes#METRIC}.
 	 * @param sgp
 	 * @return
 	 */
  	 public static int getMetric(SceneGraphPath sgp) {
 		EffectiveAppearance eap = EffectiveAppearance.create(sgp);
 		int sig = eap.getAttribute(CommonAttributes.METRIC, Pn.EUCLIDEAN);
 		return sig;
 	}
 	
  	/**
  	 * Return list of paths from <i>rootNode</i> to an instance of {@link Light}.
  	 * @param rootNode
  	 * @return
  	 */
  	 public static List<SceneGraphPath> collectLights(SceneGraphComponent rootNode) {
  	    return (List<SceneGraphPath>) new LightCollector(rootNode).visit();
  	}
  	
   	/**
   	 * Return list of paths from <i>rootNode</i> to an instance of {@link ClippingPlane},
   	 * @param rootNode
   	 * @return
   	 */
 	 public static List<SceneGraphPath> collectClippingPlanes(SceneGraphComponent rootNode) {
  	    return (List<SceneGraphPath>) new ClippingPlaneCollector(rootNode).visit();
  	}
    
  	 public static List<SceneGraphPath> getPathsBetween(final SceneGraphComponent begin, final SceneGraphNode end) {
      final PathCollector.Matcher matcher = new PathCollector.Matcher() {
        public boolean matches(SceneGraphPath p) {
          return p.getLastElement() == end;
        }
      };
      return new PathCollector(matcher, begin).visit();
    }
  	 
    /**
     * Find and return all paths fomr <i>root</i> to node with name <i>name</i>.
     * @param root
     * @param name
     * @return
     */
  	 public static List<SceneGraphPath> getPathsToNamedNodes(final SceneGraphComponent root, final String name) {
      final PathCollector.Matcher matcher = new PathCollector.Matcher() {
        public boolean matches(SceneGraphPath p) {
 //         System.out.println("compare="+p);
          return p.getLastElement().getName().equals(name);
        }
      };
      return (List<SceneGraphPath>) new PathCollector(matcher, root).visit();
    }
    
    /**
     * Remove a child of arbitrary type.
     * 
     * @param node the child to remove
     * @throws IllegalArgumentException if node is no child
     */
    public static void removeChildNode(final SceneGraphComponent parent, SceneGraphNode node) {
      node.accept(new SceneGraphVisitor() {
        public void visit(Appearance a) {
          if (parent.getAppearance() == a) parent.setAppearance(null);
          else throw new IllegalArgumentException("no such child!");
        }

        public void visit(Camera c) {
          if (parent.getCamera() == c) parent.setCamera(null);
          else throw new IllegalArgumentException("no such child!");
        }

        public void visit(Geometry g) {
          if (parent.getGeometry() == g) parent.setGeometry(null);
          else throw new IllegalArgumentException("no such child!");
        }

        public void visit(Light l) {
          if (parent.getLight() == l) parent.setLight(null);
          else throw new IllegalArgumentException("no such child!");
        }

        public void visit(Transformation t) {
          if (parent.getTransformation() == t) parent.setTransformation(null);
          else throw new IllegalArgumentException("no such child!");
        }

        public void visit(SceneGraphComponent c) {
          if (parent.getChildNodes().contains(c)) parent.removeChild(c);
          else throw new IllegalArgumentException("no such child!");
        }
      });
    }
    
    /**
     * method to add a child of arbitrary type
     * 
     * @param node the child to add
     */
    public static void addChildNode(final SceneGraphComponent parent, SceneGraphNode node) {
      node.accept(new SceneGraphVisitor() {
        public void visit(Appearance a) {
          parent.setAppearance(a);
        }

        public void visit(Camera c) {
          parent.setCamera(c);
        }

        public void visit(Geometry g) {
          parent.setGeometry(g);
        }

        public void visit(Light l) {
          parent.setLight(l);
        }

        public void visit(Transformation t) {
          parent.setTransformation(t);
        }

        public void visit(SceneGraphComponent c) {
          parent.addChild(c);
        } 
      });
    }

    
    /**
     * Linear search for the index of <i>child<i> in childlist of <i>parent</i>. 
     * Can be overridden
     * if there is a more efficient way of determining the index.
    * @param parent
    * @param child
    * @return  index, or -1 if not found.
    */public static int getIndexOfChild(SceneGraphComponent parent, SceneGraphComponent child)
    {
      final int l = parent.getChildComponentCount();
      for(int i=0; i<l; i++)
        if(parent.getChildComponent(i) == child) return i;
      return -1;
    }
    
    
    /**
     * Return a copy of the scene graph node <i>template</i>.
     * For a SceneGraphComponent, it does not include copies of the children.  
     * @param template
     * @return the copy
     * @see CopyVisitor
     */
    public static <T extends SceneGraphNode> T copy(T template) {
      CopyVisitor cv = new CopyVisitor();
//      cv.visit(template);
      template.accept(cv);
      return (T) cv.getCopy();
    }

	public static Geometry getFirstGeometry(SceneGraphComponent sgc) {
		class GetFirstGeometryVisitor extends SceneGraphVisitor {
			boolean found = false;
			Geometry geom = null;
			@Override
			public void visit(Geometry g) {
				if (found) return;
				geom = g;
				found = true;
			}

			@Override
			public void visit(SceneGraphComponent c) {
				if (found) return;
				c.childrenAccept(this);	
			}
			Geometry getGeometry()	{
				return geom;
			}
			
		}
		
		GetFirstGeometryVisitor gfgv = new GetFirstGeometryVisitor();
		gfgv.visit(sgc);
		return gfgv.getGeometry();
	}

	public static SceneGraphComponent flatten(SceneGraphComponent sgc)		{
		 return SceneGraphUtility.flatten(sgc, false);
	 }


	/**
		 * Apply transformations recursively to all instances of {@link PointSet} and
		 * produce a flat scene graph with no transformations.  
		 * It collects these instances, and transforms them into world coordinates. 
		 * All these instances are put into one parent, and this parent is returned. 
		 * Geometry that is not PointSet is simply ignored. Attributes are copied as much
		 * as possible, normals are also transformed.  The code is not robust.
		 * @param sgc
		 * @param rejectInvis	if true, non-visible scene graph components are skipped (default: false)
		 * @return
		 */
	 public static SceneGraphComponent flatten(SceneGraphComponent sgc, final boolean rejectInvis)		{
		 return flatten(sgc, rejectInvis, true);
	 }
	 public static SceneGraphComponent flatten(SceneGraphComponent sgc, final boolean rejectInvis, final boolean removeTform)		{
		    final double[] flipit = P3.makeStretchMatrix(null, new double[] {-1,0, -1,0, -1.0});
			final ArrayList geoms = new ArrayList();
			//TODO evaluate the appearance also and stick it in the flattened node with the geometry.
		    SceneGraphVisitor v =new SceneGraphVisitor() {
		    	    SceneGraphPath thePath = new SceneGraphPath();
		    	    
	            public void visit(PointSet oldi) {
	            	// have to copy the geometry in case it is reused!
	            	PointSet geometry = (PointSet) copy(oldi);
	            	//System.err.println("point set is "+i);
	            	if (geometry.getVertexAttributes(Attribute.COORDINATES) == null) return;
	            	 double[][] v = null, nv = null;
		            double[] currentMatrix = thePath.getMatrix(null);
	            	 if (removeTform)	{
		           	    v = geometry.getVertexAttributes(Attribute.COORDINATES).toDoubleArrayArray(null);
		            	nv = Rn.matrixTimesVector(null, currentMatrix, v);
		            	geometry.setVertexAttributes(Attribute.COORDINATES, StorageModel.DOUBLE_ARRAY.array(nv[0].length).createWritableDataList(nv));        		
	            	}
	                double[] cmp = null;
	         	    if (geometry instanceof IndexedFaceSet)	{
	         	    	IndexedFaceSet ifs =  (IndexedFaceSet) geometry;
	         	    	ifs.setName(ifs.getName()+"Copy");
	                    double[] mat = Rn.transpose(null, currentMatrix);          	
	                    mat[12] = mat[13] = mat[14] = 0.0;
	                    Rn.inverse(mat, mat);
	                    double det = Rn.determinant(mat);
	//             	   if (Rn.determinant(currentMatrix) < 0.0)	cmp = Rn.times(null, flipit, mat);
	//             	   else 
	             	   cmp = mat;
	            	   if (ifs.getFaceAttributes(Attribute.NORMALS) != null)	{
	            		   //System.out.println("Setting face normals");
	            		   v = ifs.getFaceAttributes(Attribute.NORMALS).toDoubleArrayArray(null);
	            		   if (removeTform)	{
		            		   nv = Rn.matrixTimesVector(null, cmp, v);
		            		   if (det < 1) Rn.times(nv, -1, nv);
		            		   ifs.setFaceAttributes(Attribute.NORMALS, StorageModel.DOUBLE_ARRAY.array(nv[0].length).createWritableDataList(nv));	            			   
	            		   } else if (Rn.determinant(currentMatrix) < 0.0 ) {
		               	   		nv = Rn.matrixTimesVector(null, flipit, v);
		               	   		ifs.setFaceAttributes(Attribute.NORMALS, StorageModel.DOUBLE_ARRAY.array(nv[0].length).createWritableDataList(nv));
	            		   }
	            	   } //else IndexedFaceSetUtility.calculateAndSetFaceNormals(ifs);
	               	   if (ifs.getVertexAttributes(Attribute.NORMALS) != null)	{
	               		   	v = ifs.getVertexAttributes(Attribute.NORMALS).toDoubleArrayArray(null);
	               		   if (removeTform) {
		           	   			//System.out.println("Setting vertex normals");
		                        nv = Rn.matrixTimesVector(null, cmp, v);
		                        if (det < 1) Rn.times(nv, -1, nv);
		                        ifs.setVertexAttributes(Attribute.NORMALS, StorageModel.DOUBLE_ARRAY.array(nv[0].length).createWritableDataList(nv));
	               		   } else if (Rn.determinant(currentMatrix) < 0.0 ) {
		               	   		nv = Rn.matrixTimesVector(null, flipit, v);	               	 		               			   
		                        ifs.setVertexAttributes(Attribute.NORMALS, StorageModel.DOUBLE_ARRAY.array(nv[0].length).createWritableDataList(nv));
	               		   }
	            	   } //else IndexedFaceSetUtility.calculateAndSetVertexNormals(ifs);
	               	   geometry = ifs;
	           	   }
	         	   //System.out.println("det is "+Rn.determinant(currentMatrix));
	//	          if (Rn.determinant(currentMatrix) < 0.0)	{
		                SceneGraphComponent foo = new SceneGraphComponent("flatfoo");
		                foo.setGeometry(geometry);
		                if (!removeTform) MatrixBuilder.euclidean(new Matrix(currentMatrix)).assignTo(foo);
		                if (thePath.getLastComponent().getAppearance() != null)	{
		                	foo.setAppearance(thePath.getLastComponent().getAppearance());
		                }
		                foo.setVisible(thePath.getLastComponent().isVisible());
		                geoms.add(foo);
		         	   	
	//          	   }
	             }
	            public void visit(SceneGraphComponent c) {
	            	if (rejectInvis && !c.isVisible()) return;
	            	thePath.push(c);
	                c.childrenAccept(this);
	               //if (c.getTransformation() != null) c.getTransformation().setMatrix(Rn.identityMatrix(4));
	               //c.setName(c.getName() + "_flat");
	                thePath.pop();
	            }
	            public void visit(Sphere s)	{
	            	    LoggingSystem.getLogger(GeometryUtility.class).log(Level.WARNING, "Can't flatten a sphere yet");
	            }
	        };
	        v.visit(sgc);
	        SceneGraphComponent flat = new SceneGraphComponent("flat"+sgc.getName());
	        if (sgc.getAppearance() != null) flat.setAppearance(sgc.getAppearance());
	        for (Iterator iter = geoms.iterator(); iter.hasNext();) {
	             SceneGraphComponent foo = (SceneGraphComponent)iter.next(); ;
	             flat.addChild(foo);
	       }
            //System.err.println("flat sgc");
	        return flat;
		}

	/**
	 * A convenience method to find the deepest occurrence of an {@link Appearance} in an
	 * instance of SceneGraphPath.
	 * @param theSelection
	 * @return
	 */
	public static Appearance findDeepestAppearance(SceneGraphPath theSelection) {
		Appearance selectedAppearance = null;
		for (Iterator lit = theSelection.reverseIterator(); lit.hasNext();) {
			Object selt = lit.next();
			if (selt != null) {
				if (selt instanceof Appearance)
					selectedAppearance = (Appearance) selt;
				else if (selt instanceof SceneGraphComponent) {
					selectedAppearance = ((SceneGraphComponent) selt)
							.getAppearance();
				}
				if (selectedAppearance != null)
					break;
			}
	 }
	 return selectedAppearance;
	}

	public static void removeLights(Viewer viewer) {
		SceneGraphComponent root = viewer.getSceneRoot();
		List<SceneGraphPath> lightpath = SceneGraphUtility.collectLights(root);
		for (SceneGraphPath sgp : lightpath) {
			Light light = (Light) sgp.getLastElement();
			SceneGraphComponent lightnode = sgp.getLastComponent();
			lightnode.setLight(null);
		}
	}

 }
