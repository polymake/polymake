package de.jreality.audio;

import java.lang.ref.WeakReference;
import java.util.Arrays;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.Map;
import java.util.Queue;

import de.jreality.math.Matrix;
import de.jreality.math.P3;
import de.jreality.math.Pn;
import de.jreality.scene.data.SampleReader;
import de.jreality.shader.CommonAttributes;
import de.jreality.shader.EffectiveAppearance;

/**
 * 
 * Sound path with delay (for Doppler shifts and such)
 * 
 * Compensates for discrepancies between video and audio frame rate
 * by low-pass filtering position information
 * 
 * @author brinkman
 *
 */
public class DelayPath implements SoundPath {

	public static final Factory FACTORY = new Factory() {
		public SoundPath newSoundPath() {
			return new DelayPath();
		}
	};

	private DistanceCueFactory directedFactory = AudioAttributes.DEFAULT_DISTANCE_CUE_FACTORY;
	private DistanceCueFactory directionlessFactory = AudioAttributes.DEFAULT_DISTANCE_CUE_FACTORY;
	private SampleProcessorFactory preProcFactory = AudioAttributes.DEFAULT_PROCESSOR_FACTORY;

	private DistanceCue distanceCue;
	private DistanceCue directionlessCue;
	private SampleProcessor preProcessor;

	private int metric = Pn.EUCLIDEAN;
	private float gain = AudioAttributes.DEFAULT_GAIN;
	private float directionlessGain = AudioAttributes.DEFAULT_DIRECTIONLESS_GAIN;
	private float speedOfSound = AudioAttributes.DEFAULT_SPEED_OF_SOUND;
	private float updateCutoff = AudioAttributes.DEFAULT_UPDATE_CUTOFF;
	private int earshot = AudioAttributes.DEFAULT_EARSHOT;
	private boolean withinEarshot = true;
	private int samplesOutOfEarshot = 0;

	private SampleReader reader;
	private int sampleRate;
	private float gamma;

	private Queue<float[]> sourceFrames = new LinkedList<float[]>();
	private Queue<Integer> frameLengths = new LinkedList<Integer>();
	private Queue<Matrix> sourcePositions = new LinkedList<Matrix>();
	private Matrix currentMicPosition, currentSourcePosition;

	private LowPassFilter rFilter, thetaFilter, phiFilter;
	private float rTarget, thetaTarget, phiTarget;
	private float rCurrent, thetaCurrent, phiCurrent;
	private float xMic, yMic, zMic;

	private Matrix auxiliaryMatrix = new Matrix();
	private double[] auxiliaryArray = new double[3];
	
	private float[] currentFrame = null;
	private int currentLength = 0;
	private int currentIndex = 0;
	private int relativeTime = 0;
	private int frameCount = 0;

	private Interpolation interpolation;

	public void initialize(SampleReader reader, Interpolation.Factory factory) {
		this.reader = reader;
		
		sampleRate = reader.getSampleRate();
		preProcessor = preProcFactory.getInstance(reader);
		distanceCue = directedFactory.getInstance(sampleRate);
		directionlessCue = directionlessFactory.getInstance(sampleRate);
		interpolation = factory.newInterpolation();

		rFilter = new LowPassFilter(sampleRate);
		thetaFilter = new LowPassFilter(sampleRate);
		phiFilter = new LowPassFilter(sampleRate);

		updateParameters();
	}

	// consider synchronization when changing this method...
	public void setProperties(EffectiveAppearance app) {
		metric = app.getAttribute(CommonAttributes.METRIC, Pn.EUCLIDEAN);
		gain = app.getAttribute(AudioAttributes.VOLUME_GAIN_KEY, AudioAttributes.DEFAULT_GAIN);
		directionlessGain = app.getAttribute(AudioAttributes.DIRECTIONLESS_GAIN_KEY, AudioAttributes.DEFAULT_DIRECTIONLESS_GAIN);
		speedOfSound = app.getAttribute(AudioAttributes.SPEED_OF_SOUND_KEY, AudioAttributes.DEFAULT_SPEED_OF_SOUND);
		updateCutoff = app.getAttribute(AudioAttributes.UPDATE_CUTOFF_KEY, AudioAttributes.DEFAULT_UPDATE_CUTOFF);
		earshot = app.getAttribute(AudioAttributes.EARSHOT_KEY, AudioAttributes.DEFAULT_EARSHOT);
		updateParameters();

		SampleProcessorFactory spf = (SampleProcessorFactory) app.getAttribute(AudioAttributes.PREPROCESSOR_KEY,
				AudioAttributes.DEFAULT_PROCESSOR_FACTORY, SampleProcessorFactory.class);
		if (spf!=preProcFactory) {
			preProcFactory = spf;
			SampleProcessor proc = spf.getInstance(reader);
			preProcessor = proc;
		}

		DistanceCueFactory dcf = (DistanceCueFactory) app.getAttribute(AudioAttributes.DISTANCE_CUE_KEY,
				AudioAttributes.DEFAULT_DISTANCE_CUE_FACTORY, DistanceCueFactory.class);
		if (dcf!=directedFactory) {
			directedFactory = dcf;
			distanceCue = dcf.getInstance(sampleRate);
		}

		dcf = (DistanceCueFactory) app.getAttribute(AudioAttributes.DIRECTIONLESS_CUE_KEY,
				AudioAttributes.DEFAULT_DISTANCE_CUE_FACTORY, DistanceCueFactory.class);
		if (dcf!=directionlessFactory) {
			directionlessFactory = dcf;
			directionlessCue = dcf.getInstance(sampleRate);
		}

		preProcessor.setProperties(app);
		distanceCue.setProperties(app);
		directionlessCue.setProperties(app);
	}

	private void updateParameters() {
		gamma = (speedOfSound>0f) ? sampleRate/speedOfSound : 0f; // samples per distance
		if (updateCutoff!=rFilter.getCutOff()) {
			rFilter.setCutOff(updateCutoff);
			thetaFilter.setCutOff(updateCutoff);
			phiFilter.setCutOff(updateCutoff);
		}
	}
	
	public boolean processFrame(SoundEncoder enc, int frameSize, Matrix sourcePosition, Matrix inverseMicMatrix, float[] directionlessBuffer) {
		currentMicPosition = inverseMicMatrix;
		currentSourcePosition = sourcePosition;
		updateTarget();
		
		boolean sourceActive = evaluateSourceFrame(frameSize);
		encodeFrame(enc, frameSize, directionlessBuffer);

		if (sourceActive || frameCount>0 || currentFrame!=null || 
				preProcessor.hasMore() || distanceCue.hasMore() || directionlessCue.hasMore()) {
			return true;   // still rendering...
		} else {
			reset();
			return false;  // nothing left to render
		}
	}

	private boolean evaluateSourceFrame(int frameSize) {
		float[] newFrame = getBuffer(frameSize);
		int nRead = preProcessor.read(newFrame, 0, frameSize);
		if (withinEarshot) { // within earshot: render audio normally
			if (nRead>0) {
				frameCount++;
				if (nRead<frameSize) {
					Arrays.fill(newFrame, nRead, frameSize, 0);
				}
				queueFrame(frameSize, newFrame);
			} else {
				reuseBuffer(newFrame);
				queueFrame(frameSize, null);
			}
			withinEarshot = (earshot<=0 || relativeTime<earshot+4*frameSize);
		} else { // out of earshot: render null frames
			reuseBuffer(newFrame);
			samplesOutOfEarshot += frameSize;
			withinEarshot = (relativeTime<earshot);
			if (withinEarshot || 2*samplesOutOfEarshot/frameSize>relativeTime/earshot) {
				queueNullFrame();
			}
		}
		return nRead>0;
	}

	private void queueNullFrame() {
		queueFrame(samplesOutOfEarshot, null);
		samplesOutOfEarshot = 0;
	}

	private void queueFrame(int size, float[] frame) {
		sourcePositions.add(new Matrix(currentSourcePosition));
		if (currentLength>0) {
			frameLengths.add(size);
			sourceFrames.add(frame);
		} else {
			currentLength = size;
			currentFrame = frame;

			rCurrent = rFilter.initialize(rTarget);
			thetaCurrent = thetaFilter.initialize(thetaTarget);
			phiCurrent = phiFilter.initialize(phiTarget);
		}
	}

	private void updateTarget() {
		auxiliaryMatrix.assignFrom(sourcePositions.isEmpty() ? currentSourcePosition : sourcePositions.element());
		auxiliaryMatrix.multiplyOnLeft(currentMicPosition);
		
		homogeneousToSpherical(auxiliaryArray, auxiliaryMatrix.getColumn(3), metric);
		rTarget = (float) auxiliaryArray[0];
		thetaTarget = (float) auxiliaryArray[1];
		phiTarget = (float) auxiliaryArray[2];

		while (thetaTarget-thetaCurrent>Math.PI) thetaCurrent += 2*Math.PI;
		while (thetaCurrent-thetaTarget>Math.PI) thetaCurrent -= 2*Math.PI;
		thetaFilter.initialize(thetaCurrent);
		
		auxiliaryMatrix.invert();

		homogeneousToSpherical(auxiliaryArray, auxiliaryMatrix.getColumn(3), metric);
		auxiliaryArray[0] = 1f; // we want a unit vector
		sphericalToRectangular(auxiliaryArray);
		xMic = (float) auxiliaryArray[0];
		yMic = (float) auxiliaryArray[1];
		zMic = (float) auxiliaryArray[2];
	}
	
	private static void homogeneousToSpherical(double[] dst, double[] src, int metric) {
		double x = src[0];
		double y = src[1];
		double z = src[2];
		dst[0] = Pn.distanceBetween(P3.originP3, src, metric);
		dst[1] = Math.atan2(x, z);
		dst[2] = Math.atan2(y, Math.sqrt(x*x+z*z));
	}
	
	private static void sphericalToRectangular(double[] p) {
		double r = p[0];
		double theta = p[1];
		double phi = p[2];
		
		double cp = r*Math.cos(phi);
		double sp = r*Math.sin(phi);
		p[2] = cp*Math.cos(theta);
		p[0] = cp*Math.sin(theta);
		p[1] = sp;
	}

	private void advanceFrame() {
		if (currentFrame!=null) {
			reuseBuffer(currentFrame);
		}
		
		currentFrame = sourceFrames.remove();
		currentLength = frameLengths.remove();
		sourcePositions.remove();
		updateTarget();

		if (currentFrame!=null) {
			frameCount--;
		}
	}

	private void encodeFrame(SoundEncoder enc, int frameSize, float[] directionlessBuffer) {
		for(int j=0; j<frameSize; j++) {
			float time = (relativeTime++)-gamma*rCurrent;
			int targetIndex = (int) time;
			float fractionalTime = time-targetIndex;

			for(; targetIndex>=currentIndex; currentIndex++) {
				if (currentIndex>=currentLength) {
					relativeTime -= currentLength;
					currentIndex -= currentLength;
					targetIndex -= currentLength;
					advanceFrame();
				}
				float newSample = (currentFrame!=null) ? currentFrame[currentIndex]*gain : 0f;
				newSample = directionlessCue.nextValue(newSample, rCurrent, xMic, yMic, zMic);
				interpolation.put(newSample);
			}

			float v = interpolation.get(fractionalTime);
			v = distanceCue.nextValue(v, rCurrent, xMic, yMic, zMic);
			
			auxiliaryArray[0] = 1f;  // we want a unit vector indicating the direction
			auxiliaryArray[1] = thetaCurrent;
			auxiliaryArray[2] = phiCurrent;
			sphericalToRectangular(auxiliaryArray);
			enc.encodeSample(v, j, rCurrent, (float) auxiliaryArray[0], (float) auxiliaryArray[1], (float) auxiliaryArray[2]);
			if (directionlessBuffer!=null) {
				directionlessBuffer[j] += v*directionlessGain;
			}

			rCurrent = rFilter.nextValue(rTarget);
			thetaCurrent = thetaFilter.nextValue(thetaTarget);
			phiCurrent = phiFilter.nextValue(phiTarget);
		}
	}

	private void reset() {
		sourceFrames.clear();
		frameLengths.clear();
		sourcePositions.clear();
		currentFrame = null;
		currentLength = 0;
		currentIndex = 0;
		relativeTime = 0;
		frameCount = 0;
		withinEarshot = true;
		samplesOutOfEarshot = 0;
		interpolation.reset();
		preProcessor.clear();
		distanceCue.reset();
		directionlessCue.reset();
	}

	private static final Map<Integer, Queue<WeakReference<float[]>>> framePool = new HashMap<Integer, Queue<WeakReference<float[]>>>();

	private static void reuseBuffer(float[] frame) {
		int size = frame.length;
		synchronized(framePool) {
			Queue<WeakReference<float[]>> queue = framePool.get(size);
			if (queue==null) {
				queue = new LinkedList<WeakReference<float[]>>();
				framePool.put(size, queue);
			}
			queue.add(new WeakReference<float[]>(frame));
		}
	}

	private static float[] getBuffer(int size) {
		synchronized (framePool) {
			Queue<WeakReference<float[]>> queue = framePool.get(size);
			if (queue!=null) {
				while (!queue.isEmpty()) {
					float[] frame = queue.remove().get();
					if (frame!=null) {
						return frame;
					}
				}
			}
		}
		return new float[size];
	}
}
