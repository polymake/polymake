package de.jreality.audio;

/**
 * Simple interface for defining interpolations of equally spaced samples; comes with implementations
 * for sample-and-hold (i.e., no interpolation) as well as linear, cosine, and cubic (4-point)
 * interpolation.
 * 
 * @author brinkman
 *
 */
public interface Interpolation {

	/**
	 * @param v  new sample to be added to list of values
	 */
	void put(float v);
	
	/**
	 * @param t  time at which to evaluate interpolation; should be between 0 and 1
	 * @return   interpolated value for time t
	 */
	float get(float t);
	
	void reset();
	
	public interface Factory {
		Interpolation newInterpolation();
	}
	
	public final class SampleHold implements Interpolation {
		public static final Factory FACTORY = new Factory () {
			public Interpolation newInterpolation() {
				return new SampleHold();
			}
		};
		private float v;

		public float get(float t) {
			return v;
		}
		public void put(float v) {
			this.v = v;
		}
		public void reset() {
			v = 0;
		}
	}

	public final class Linear implements Interpolation {
		public static final Factory FACTORY = new Factory () {
			public Interpolation newInterpolation() {
				return new Linear();
			}
		};
		private float v, dv;
		
		public float get(float t) {
			return v+t*dv;
		}
		public void put(float v) {
			this.v += dv;
			dv = v - this.v;
		}
		public void reset() {
			v = dv = 0;
		}
	}

	public final class Cosine implements Interpolation {
		public static final Factory FACTORY = new Factory () {
			public Interpolation newInterpolation() {
				return new Cosine();
			}
		};
		private float v, dv;

		public float get(float t) {
			float s = (float) (1-Math.cos(t*Math.PI))/2;
			return v+s*dv;
		}
		public void put(float v) {
			this.v += dv;
			dv = v - this.v;
		}
		public void reset() {
			v = dv = 0;
		}
	}
	
	public final class Cubic implements Interpolation {
		public static final Factory FACTORY = new Factory () {
			public Interpolation newInterpolation() {
				return new Cubic();
			}
		};
		private float v0, v1, v2, v3, a0, a1, a2, a3;
		private boolean dirty;

		public float get(float t) {
			if (dirty) {
				a0 = v3 - v2 - v0 + v1;
				a1 =           v0 - v1 - a0;
				a2 =      v2 - v0;
				a3 =                v1;
				dirty = false;
			}
			float t2 = t*t;
			return(a0*t*t2+a1*t2+a2*t+a3);
		}
		public void put(float v) {
			v0 = v1;
			v1 = v2;
			v2 = v3;
			v3 = v;
			dirty = true;
		}
		public void reset() {
			v0 = v1 = v2 = v3 = 0;
			dirty = true;
		}
	}
}
