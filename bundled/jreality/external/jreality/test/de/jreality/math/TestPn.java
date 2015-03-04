package de.jreality.math;

import static org.junit.Assert.assertTrue;

import org.junit.Test;

public class TestPn {

	@Test
	public void testProjectivityToCanonical() {
		double[][] dm = {{1,2,3}, {1,0,-1},{0,2,1},{3,7,1}};
		double[] c2p = Pn.projectivityFromCanonical(null, dm);
		double[] foo = Rn.matrixTimesVector(null, c2p, new double[]{1,1,1});
		assertTrue(Pn.isEquivalentPoints(foo, dm[3]));
	}

	@Test
	public void testProjectivity() {
		double[][] dm = {{1,2,3}, {-1,0,1},{0,2,1},{3,7,1}};
		double[][] im = {{2,-3,0},{0,1,4}, {-3,0,3}, {1,2,3}};
		double[] proj = Pn.projectivity(null, dm, im);
		for (int i = 0; i<dm.length; ++i)	{
			double[] foo = Rn.matrixTimesVector(null, proj, dm[i]);
			assertTrue(Pn.isEquivalentPoints(foo, im[i]));
		}
	}

}
