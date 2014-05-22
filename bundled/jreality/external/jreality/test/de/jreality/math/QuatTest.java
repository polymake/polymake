package de.jreality.math;

import java.util.Random;

import junit.framework.TestCase;

public class QuatTest extends TestCase {

	Random rand;
	
	@Override
	protected void setUp() throws Exception {
		rand = new Random(31);
	}
	
	private double[] randQuat(double l) {
		return new double[] {
				l*(-0.5+rand.nextDouble()),
				l*(-0.5+rand.nextDouble()),
				l*(-0.5+rand.nextDouble()),
				l*(-0.5+rand.nextDouble())
		};
	}
	
	public void testSubtract() {
		for (int i=0; i<100; i++) {
			double[] rq = randQuat(0.1*i);
			double[] rqq = rq.clone();
			double[] zero = Quat.subtract(null, rq, rqq);
			assertEquals(0.0, Quat.lengthSqared(zero));
		}
	}
	
	public void testTimesWithIdentity() {
		double[] q1 = Quat.toQuat(null, 1, 0,0,0);
		for (int i=0; i<100; i++) {
			double[] rq = randQuat(0.1*i);
			double[] res1 = Quat.times(null, q1, rq);
			double[] res2 = Quat.times(null, rq, q1);
			double[] zero1 = Quat.subtract(null, rq, res1);
			double[] zero2 = Quat.subtract(null, rq, res2);
			assertEquals(0.0, Quat.lengthSqared(zero1), 1E-12);
			assertEquals(0.0, Quat.lengthSqared(zero2), 1E-12);
		}
	}

	public void testInverse() {
		for (int i=0; i<100; i++) {
			double[] rq = randQuat(0.1*(i+1));
			double[] rq_inv = Quat.invert(null, rq);
			double[] one = Quat.times(null, rq, rq_inv);
			assertEquals(1.0, one[0], 1E-12);
			assertEquals(0.0, one[1], 1E-12);
			assertEquals(0.0, one[2], 1E-12);
			assertEquals(0.0, one[3], 1E-12);
		}		
	}
	
	public void testConjugateAndInverse() {
		for (int i=0; i<100; i++) {
			double[] rq = randQuat(0.1*(i+1));
			double[] r_inv = Quat.invert(null, rq);
			double[] bar = Quat.conjugate(null, rq);
			double l = Quat.length(rq);
			double ll = l*l;
			double[] r_inv2 = Quat.times(null, 1./ll, bar);
			double[] zero = Quat.subtract(null, r_inv, r_inv2);
			assertEquals(0.0, Quat.lengthSqared(zero), 1E-12);
		}
	}
	
}
