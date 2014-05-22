package de.jreality.util;

import java.util.WeakHashMap;

import de.jreality.math.Rn;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SceneGraphVisitor;
import de.jreality.scene.Transformation;

/**
 * Utility to store default matrices for {@link Transformation}s.
 * For convienience, there is one shared instance.
 * 
 * @author weissman
 *
 */
public class DefaultMatrixSupport {

	private static final double[] IDENTITY = Rn.identityMatrix(4);
	private WeakHashMap<Transformation, double[]> store = new WeakHashMap<Transformation, double[]>();

	private static final DefaultMatrixSupport instance = new DefaultMatrixSupport();
	
	public static DefaultMatrixSupport getSharedInstance() {
		return instance;
	}
	
	/**
	 * Stores the given array as default matrix for trafo.
	 * @param trafo the trafo for which to set the default matrix
	 * @param defMatrix the default matrix for trafo
	 */
	public void storeDefault(Transformation trafo, double[] defMatrix) {
		store.put(trafo, defMatrix);
	}
	
	/**
	 * Stores the current matrix of trafo as its default matrix.
	 * @param trafo the trafo
	 */
	public void storeAsDefault(Transformation trafo) {
		storeDefault(trafo, trafo.getMatrix());
	}
	
	/**
	 * Restores the default matrix if there is any, otherwise
	 * assigns the identity matrix or does nothing, depending on the
	 * <code>clear</code> flag.
	 * @param trafo the trafo to restore
	 * @param clear determines the behaviour for Transformations
	 * without a default matrix: if true, set to the indentity - if
	 * false, do nothing. 
	 */
	public void restoreDefault(Transformation trafo, boolean clear) {
		double[] defMatrix = store.get(trafo);
		if (defMatrix != null && Rn.equals(defMatrix, trafo.getMatrix())) return;
		if (defMatrix == null && !clear) return;
		if (!trafo.isReadOnly()) trafo.setMatrix(defMatrix != null ? defMatrix : IDENTITY);
	}
	
	/**
	 * Traverses thetree from the given root and calls {@link restoreDefault}
	 * for all {@link Transformation}s.
	 * @param root the root of the subgraph to traverse.
	 * @param clear determines the behaviour for Transformations
	 * without a default matrix: if true, set to the indentity - if
	 * false, do nothing. 
	 */
	SceneGraphComponent cc;
  	public void restoreDefaultMatrices(SceneGraphComponent root, final boolean clear) {
	  	root.accept(new SceneGraphVisitor() {
	  		@Override
	  		public void visit(SceneGraphComponent c)	{
	  			cc = c;
			  	c.childrenWriteAccept(this, true, false, false, false, false, false);
		  	}
		  	@Override
		  	public void visit(Transformation t) {
		  		restoreDefault(t, clear);
		  	}
	  	});
  	}
  
	
	/**
	 * Traverses thetree from the given root and calls {@link storeDefault}
	 * for all {@link Transformation}s.
	 * @param root the root of the subgraph to traverse
	 */
 	public void storeDefaultMatrices(SceneGraphComponent root)	{
	  	root.accept(new SceneGraphVisitor() {
	  		@Override
	  		public void visit(SceneGraphComponent c)	{
			  	c.childrenAccept(this);
		  	}
		  	@Override
		  	public void visit(Transformation t) {
		  		storeAsDefault(t);
		  	}
	  	});
  	}

}
