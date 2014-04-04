package de.jreality.junitutils;


import java.util.Arrays;

import org.junit.Ignore;

@Ignore
public class Assert extends org.junit.Assert {
	
	public static void assertArrayEquals(double[] expected, double[] actual, double delta) {
		if (expected.length != actual.length) fail(expected, actual);
		for (int i = 0; i < expected.length; i++) {
			if (Math.abs(expected[i] - actual[i]) > delta) {
				fail(expected, actual);
			}
		}
	}
	
	private static void fail(double[] expected, double[] actual) {
		org.junit.Assert.fail("expected:<" + Arrays.toString(expected) + "> but was:<" + Arrays.toString(actual) + ">");
	}
	
	public static void assertDifferent(double notExpected, double actual, double delta) {
		assertDifferent("", notExpected, actual, delta);
	}
	
	public static void assertDifferent(String message, double notExpected, double actual, double delta) {
		if (Math.abs(notExpected - actual) < delta )
			fail(message + "<" + notExpected + ">" + " expected to be different form <" + actual +">"); 
	}


}
