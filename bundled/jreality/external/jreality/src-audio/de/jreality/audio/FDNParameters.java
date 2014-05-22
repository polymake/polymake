package de.jreality.audio;

/**
 * Parameters (number of delay lines, delay times, feedback matrix, gains) for a feedback delay network
 * ({@link FDNReverb}).
 * 
 * @author brinkman
 *
 */
public interface FDNParameters {

	public static final FDNParameters BUNNY_PARAMETERS = new FDNParameters() {
		private float[] times = {.021f, .034f, .055f, .089f, .144f, .233f, .377f, .610f};  // from Fibonacci sequence...
		private float[] gains = new float[8];
		private float[] tmpBuffer = new float[8];
		private float reverbTime;
		
		{
			setReverbTime(AudioAttributes.DEFAULT_REVERB_TIME);
		}

		public float delayTime(int i) {
			return times[i];
		}
		public void map(float[] result, float[] argument) {  // Sylvester's construction of Hadamard matrices
			iterativeStep(result, argument);
			iterativeStep(tmpBuffer, result);
			iterativeStep(result, tmpBuffer);
			for(int i=0; i<8; i++) {
				result[i] *= gains[i];
			}
		}
		private void iterativeStep(float[] result, float[] argument) {
			for(int i=0; i<3; i++) {
				for(int j=0; j<4; j++) {
					result[j] = argument[2*j]+argument[2*j+1];
					result[4+j] = argument[2*j]-argument[2*j+1];
				}
			}
		}
		public int numberOfLines() {
			return 8;
		}
		public float getReverbTime() {
			return reverbTime;
		}
		public void setReverbTime(float t) {
			reverbTime = t;
			float q = (float) Math.log(.001)/reverbTime;
			for(int i=0; i<8; i++) {
				gains[i] = (float) (Math.exp(q*times[i])/Math.sqrt(8));
			}
		}
	};
	
	public int numberOfLines();
	public float delayTime(int i);
	public void map(float[] result, float[] argument);
	public float getReverbTime();
	public void setReverbTime(float t);
}
