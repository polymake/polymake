package de.jreality.math;

import junit.framework.TestCase;

public class QuaternionTest extends TestCase {
	
	public void testRotationMatrixToQuaternion() {
		assertTrue(Quaternion.equals(new Quaternion(1,0,0,0), 
			Quaternion.rotationMatrixToQuaternion(null, 
				new double[]{1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0}),1E-10));
	}

}
