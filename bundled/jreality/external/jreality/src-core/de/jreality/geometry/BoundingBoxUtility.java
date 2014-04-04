package de.jreality.geometry;

import de.jreality.math.Pn;
import de.jreality.math.Rn;
import de.jreality.scene.PointSet;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.Sphere;
import de.jreality.scene.data.Attribute;
import de.jreality.util.Rectangle3D;

/** A set of static methods for calculating rectangular bounding boxes in euclidean space 
 * 
 * @author gunn
 *
 */
public class BoundingBoxUtility {

	/**
	 * Calculate the bounding box assuming that the scene graph is first transformed by
	 * the matrix <i>initialMatrix</i>
	 * @param tmp
	 * @param sgc
	 * @return
	 */
	public static Rectangle3D calculateBoundingBox(double[] initialMatrix, SceneGraphComponent sgc) {
		BoundingBoxTraversal bbt = new BoundingBoxTraversal();
		if (initialMatrix!=null) bbt.setInitialMatrix(initialMatrix);
		bbt.traverse(sgc);
		if (Double.isNaN(bbt.getBoundingBox().getBounds()[0][0]))
			throw new IllegalStateException("NaN in calculateBoundingBox");
			//return Rectangle3D.EMPTY_BOX;
		return bbt.getBoundingBox();
	}

	/**
	 * Calculate the bounding box of the vertices <i>verts</i>. These may be
	 * 3- or 4-d points. 
	 * @param verts
	 * @return
	 * {@see Pn} for details.
	 */
	 public static Rectangle3D calculateBoundingBox(double[][] verts)	{
		double[][] bnds = new double[2][3];
		if (verts[0].length == 4)	{
			Pn.calculateBounds(bnds, verts);
		} else {
			Rn.calculateBounds(bnds, verts);
		}
		Rectangle3D r3d = new Rectangle3D();
		r3d.setBounds(bnds);
		return r3d;
	}

	public static Rectangle3D calculateBoundingBox(PointSet ps)	{
	       Object bbox = ps.getGeometryAttributes(GeometryUtility.BOUNDING_BOX);
	        if (bbox != null && bbox instanceof Rectangle3D)    {
	            System.err.println("found bbox as GA");
	            return ((Rectangle3D) bbox);
	        }
		double[][] verts = ps.getVertexAttributes(Attribute.COORDINATES).toDoubleArrayArray(null);
		return calculateBoundingBox(verts);
	}

	public static Rectangle3D calculateBoundingBox(SceneGraphComponent sgc)	{
		return calculateBoundingBox(null, sgc);
	}

	public static Rectangle3D calculateBoundingBox(Sphere sph)	{
		return SphereUtility.getSphereBoundingBox();
	}

	/**
	 * Calculate the bounding box for the scene graph tooted at <i>sgc</i> but
	 * do not apply the transformation, if any, attached to <i>sgc</i>.
	 * @param sgc
	 * @return
	 */
	public static Rectangle3D calculateChildrenBoundingBox(SceneGraphComponent sgc)	{
		SceneGraphComponent tmp = new SceneGraphComponent();
		for (int i =0; i<sgc.getChildComponentCount(); ++i)	
			tmp.addChild(sgc.getChildComponent(i));
		
		tmp.setGeometry(sgc.getGeometry());
		return calculateBoundingBox(null, tmp);
	}

	
	/**
	 * Adds a small value to a dimension of zero extend
	 * @param r
	 * @return
	 */
	public static Rectangle3D removeZeroExtends(Rectangle3D r) {
		double[] e = r.getExtent();
		double[][] bounds = r.getBounds();
		if (e[0] < 1E-20) {
			bounds[0][0] = -1E-5;
			bounds[1][0] = 1E-5;
		} 
		if (e[1] < 1E-20) {
			bounds[0][1] = -1E-5;
			bounds[1][1] = 1E-5;
		}
		if (e[2] < 1E-20) {
			bounds[0][2] = -1E-5;
			bounds[1][2] = 1E-5;
		}
		Rectangle3D result = new Rectangle3D();
		result.setBounds(bounds);
		return result;
	}
	
	
}
