/*
 * Created on Jun 23, 2004
 *
 */
package de.jreality.geometry;

import de.jreality.math.Rn;
import de.jreality.scene.IndexedFaceSet;

/**
 * This class implements Bezier tensor product surfaces of arbitrary dimension.  The
 * underlying Bezier curves can be arbitrary degree, and must not be the same.  For example,
 * it is possible to have quadratic curves in the u parameter direction and cubics in the
 * v direction.  
 * <p>
 * Instances are immutable.  The only available constructor (see {@link #BezierPatchMesh(int, int, double[][][])}
 * specifies the two degrees (u and v respectively) and an array of control points. Each row
 * of control points specifies one u-curve; each column specifies a v-curve. The control points
 * can be n-dimensional. The
 * control points must be consistent with the degree specification.  That is, the number of 
 * control points in the u-direction must be 1 more than a multiple of the u-degree; and similarly 
 * for v.   The simplest case is a single patch; for a cubic Bezier, this is a 4x4 array of
 * control points.
 * <p>
 * Currently the only operations supported on the mesh is binary refinement: {@link #refineU()},
 * {@link #refineV()}, and {@link #refine()}.  
 * <p>
 * TODO: add a tessellate method which creates an nxn QuadMesh from each set of control points
 * in the mesh.
 * <p>
 * TODO: implement adaptive refinement.  This will require a new data structure probably a quad-tree
 * to store the differently-refined levels of the original patch mesh.
 * @author Charles Gunn
 *
 */
public class BezierPatchMesh {
	double[][][] controlPoints;
	int uDegree, vDegree;
	double[] u0Split, u1Split, v0Split, v1Split;	// matrices expressing subdivision step 
	
	public BezierPatchMesh(int uDegree, int vDegree, double[][][] cp)	{
		super();
		this.uDegree = uDegree;
		this.vDegree = vDegree;
		int foo = cp.length;
		int bar = cp[0].length;

		if ((vDegree > 1 && foo != 2 && cp.length % (vDegree) != 1) || (uDegree > 1 && bar != 2 && cp[0].length % (uDegree) != 1))	{
			throw new IllegalArgumentException("Array length must be for form degree*n + 1");
		}
		controlPoints = cp;
		u0Split = Rn.identityMatrix(uDegree+1);
		u1Split = Rn.identityMatrix(uDegree+1);
		v0Split = Rn.identityMatrix(vDegree+1);
		v1Split = Rn.identityMatrix(vDegree+1);
		
		double[][] ptri = {{1},{1,1},{1,2,1},{1,3,3,1},{1,4,6,4,1},{1,5,10,10,5,1}};
		
		double factor = 1.0;
		int size = (uDegree+1)*(uDegree+1);
		for (int i = 0; i<= uDegree; ++i)	{
			for (int j = 0; j<=i; ++j)	{
				u0Split[i*(uDegree+1) + j] = factor * ptri[i][j];
				u1Split[size - i*(uDegree+1) - j - 1] = factor * ptri[i][j];
			}
			factor *= .5;
		}
		factor = 1.0;
		size = (vDegree+1)*(vDegree+1);
		for (int i = 0; i<= vDegree; ++i)	{
			for (int j = 0; j<=i; ++j)	{
				v0Split[i*(vDegree+1) + j] = factor * ptri[i][j];
				v1Split[size - i*(vDegree+1) - j - 1] = factor * ptri[i][j];
			}
			factor *= .5;
		}
		
	}
	
	
	public  void refineU()	{
		int vDim = controlPoints.length;
		int uDim = controlPoints[0].length;
		
		int vectorLength = controlPoints[0][0].length;
		double[][][] vals = new double[vDim][2*uDim-1][vectorLength];
		double[] icp = new double[uDegree+1];
		double[] ocp = new double[uDegree+1];
		for (int k = 0; k< vDim; ++k)	{
			for (int i = 0; i<vectorLength; ++i)	{
				int outCount = 0;
				for (int inCount = 0; inCount < uDim-1; inCount += uDegree)	{
					for (int j = 0; j<=uDegree; ++j)		icp[j] = controlPoints[k][inCount+j][i];
					Rn.matrixTimesVector(ocp, u0Split, icp);
					for (int j = 0; j<=uDegree; ++j)		vals[k][outCount+j][i] = ocp[j];
					outCount += uDegree;
					Rn.matrixTimesVector(ocp, u1Split, icp);
					for (int j = 0; j<=uDegree; ++j)		vals[k][outCount+j][i] = ocp[j];
					outCount += uDegree;
				}
			} 
		}
		controlPoints = vals;		
	}
	public  void refineV()	{
		int vDim = controlPoints.length;
		int uDim = controlPoints[0].length;
		int vectorLength = controlPoints[0][0].length;
		double[][][] vals = new double[2*vDim-1][uDim][vectorLength];
		double[] icp = new double[vDegree+1];
		double[] ocp = new double[vDegree+1];
		for (int k = 0; k< uDim; ++k)	{
			for (int i = 0; i<vectorLength; ++i)	{
				int outCount = 0;
				for (int inCount = 0; inCount < vDim-1; inCount += vDegree)	{
					for (int j = 0; j<=vDegree; ++j)		icp[j] = controlPoints[inCount+j][k][i];
					Rn.matrixTimesVector(ocp, v0Split, icp);
					for (int j = 0; j<=vDegree; ++j)		vals[outCount+j][k][i] = ocp[j];
					outCount += vDegree;
					Rn.matrixTimesVector(ocp, v1Split, icp);
					for (int j = 0; j<=vDegree; ++j)		vals[outCount+j][k][i] = ocp[j];
					outCount += vDegree;
				}
			} 
		}
		controlPoints = vals;		
	}
	
	/**
	 * 
	 */
	public void refine() {
		refineU();
		refineV();
	}

	/**
	 * @return Returns the controlPoints.
	 */
	public double[][][] getControlPoints() {
		return controlPoints;
	}
	/**
	 * @return Returns the uDegree.
	 */
	public int getUDegree() {
		return uDegree;
	}
	/**
	 * @return Returns the vDegree.
	 */
	public int getVDegree() {
		return vDegree;
	}

	public static IndexedFaceSet representBezierPatchMeshAsQuadMesh(BezierPatchMesh bpm)	{
		return representBezierPatchMeshAsQuadMeshFactory(null, bpm, 0).getIndexedFaceSet();
	}

	public static QuadMeshFactory representBezierPatchMeshAsQuadMesh(BezierPatchMesh bpm, int metric)	{
		QuadMeshFactory qmf = new QuadMeshFactory();
		representBezierPatchMeshAsQuadMeshFactory(qmf, bpm, metric);
		return qmf;
	}

//	public static IndexedFaceSet representBezierPatchMeshAsQuadMesh(IndexedFaceSet existing, BezierPatchMesh bpm, int metric)	{
//		return representBezierPatchMeshAsQuadMesh(null, existing, bpm, metric).getIndexedFaceSet();
//	}
	
	public static QuadMeshFactory representBezierPatchMeshAsQuadMeshFactory(QuadMeshFactory qmf,  BezierPatchMesh bpm, int metric)	{
		double[][][] thePoints = bpm.getControlPoints();
		//if (qmpatch == null) 
		if (qmf == null) qmf = new QuadMeshFactory();
		qmf.setMetric(metric); 
		qmf.setULineCount(bpm.uDegree == 1 ? thePoints[0].length : (thePoints[0].length/bpm.uDegree)+1);
		qmf.setVLineCount(bpm.vDegree == 1 ? thePoints.length : (thePoints.length/bpm.vDegree)+1);
		qmf.setClosedInUDirection(false);
		qmf.setClosedInVDirection(false);
		qmf.setGenerateTextureCoordinates(true);
	    double[] verts1d = Rn.convertArray3DToArray1D(thePoints, bpm.uDegree, bpm.vDegree);
	    qmf.setVertexCoordinates(verts1d);
	    qmf.setGenerateFaceNormals(true);
	    qmf.setGenerateVertexNormals(true);
	    qmf.update();
		return qmf;
	}
}