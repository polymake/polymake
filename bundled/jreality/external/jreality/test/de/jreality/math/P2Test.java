package de.jreality.math;

import org.junit.Assert;
import org.junit.Test;

public class P2Test {

	@Test
	public void testPerpendicularBisector() {
		double[] p1 = {0.5,0,1};
		double[] p2 = {1,0,2};
		double[] q1 = {0,0.5,1};
		double[] b1 = P2.perpendicularBisector(null, p1, q1, Pn.EUCLIDEAN);
		double[] b2 = P2.perpendicularBisector(null, p2, q1, Pn.EUCLIDEAN);
		Assert.assertEquals(0.0, b1[0]/b2[0] - b1[1]/b2[1], 1E-10);
	}
	
	@Test
	public void testPerpendicularBisectorIntersection() {
		double[] p1 = {1.123815689564646, 0.9705359505519398, 1.0433499263326802};
		double[] q1 = {0.4929383465735829, 0.7318392264919052, 1.0433499263326802};//{0,1,1};
		double[] p2 = {1.7102470752687353, 0.4401733843304918, 0.9615937451102385};
		double[] q2 = {0.0, 0.0, 0.9615937451102385}; //{0,1.5,1};
		double[] o = P2.pointFromLines(null, P2.perpendicularBisector(null, p1, q1, Pn.EUCLIDEAN), P2.perpendicularBisector(null, p2, q2, Pn.EUCLIDEAN));
		Assert.assertEquals(Pn.distanceBetween(p1, o, Pn.EUCLIDEAN), Pn.distanceBetween(q1, o, Pn.EUCLIDEAN), 1E-10);
		Assert.assertEquals(Pn.distanceBetween(p2, o, Pn.EUCLIDEAN), Pn.distanceBetween(q2, o, Pn.EUCLIDEAN), 1E-10);
	}
	
	@Test
	public void testPerpendicularBisectorIntersectionHomogeneous() {
		double[] p1 = {0.5,0,1};
		double[] q1 = {0,1,1};
		double[] p2 = {1,0,1};
		double[] q2 = {0,1.5,1};
		Rn.times(p1, 2, p1);
		double[] o = P2.pointFromLines(null, 
				P2.perpendicularBisector(null, p1, q1, Pn.EUCLIDEAN), 
				P2.perpendicularBisector(null, p2, q2, Pn.EUCLIDEAN));
		Assert.assertEquals(Pn.distanceBetween(p1, o, Pn.EUCLIDEAN), Pn.distanceBetween(q1, o, Pn.EUCLIDEAN), 1E-10);
		Assert.assertEquals(Pn.distanceBetween(p2, o, Pn.EUCLIDEAN), Pn.distanceBetween(q2, o, Pn.EUCLIDEAN), 1E-10);
	}
}
