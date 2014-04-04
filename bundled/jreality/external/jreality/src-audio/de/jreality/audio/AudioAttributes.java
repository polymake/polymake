package de.jreality.audio;

import de.jreality.scene.data.SampleReader;

public class AudioAttributes {
	private AudioAttributes() {}; // not to be instantiated
	
	public static final String PREPROCESSOR_KEY = "preprocessorKey";
	public static final String DIRECTIONLESS_PROCESSOR_KEY = "directionlessProcessor";
	public static final String REVERB_TIME_KEY = "reverbTime";
	public static final String SPEED_OF_SOUND_KEY = "speedOfSound";
	public static final String VOLUME_GAIN_KEY = "volumeGain";
	public static final String DIRECTIONLESS_GAIN_KEY = "directionlessVolumeGain";
	public static final String DISTANCE_CUE_KEY = "distanceCue";
	public static final String DIRECTIONLESS_CUE_KEY = "directionlessCueKey";
	public static final String UPDATE_CUTOFF_KEY = "updateCutoff";
	public static final String FDN_PARAMETER_KEY = "fdnParameters";
	public static final String DISTANCE_LOWPASS_KEY = "distanceLowPassKey";
	public static final String PITCH_SHIFT_KEY = "pitchShiftKey";
	public static final String EARSHOT_KEY = "earshotKey";
	
	public static final float DEFAULT_REVERB_TIME = 1.5f;
	public static final float DEFAULT_GAIN = 1f;
	public static final float DEFAULT_DIRECTIONLESS_GAIN = 0.1f;
	public static final float DEFAULT_SPEED_OF_SOUND = 332f;
	public static final float DEFAULT_UPDATE_CUTOFF = 6f; // play with this parameter if audio gets choppy
	public static final FDNParameters DEFAULT_FDN_PARAMETERS = FDNParameters.BUNNY_PARAMETERS;
	public static final float DEFAULT_DISTANCE_LOWPASS_FREQ = 44000;
	public static final float DEFAULT_PITCH_SHIFT = 1f;
	public static final Interpolation.Factory DEFAULT_INTERPOLATION_FACTORY = Interpolation.Cubic.FACTORY;
	public static final SoundPath.Factory DEFAULT_SOUNDPATH_FACTORY = DelayPath.FACTORY;
	public static final int DEFAULT_EARSHOT = 96000; // measured in samples; zero or less means infinite; default 2sec @ 48000kHz
	
	public static final DistanceCueFactory DEFAULT_DISTANCE_CUE_FACTORY = new DistanceCueFactory() {
		private final DistanceCue cue = new DistanceCue.CONSTANT();
		public DistanceCue getInstance(float sampleRate) {
			cue.setSampleRate(sampleRate);
			return cue;
		}
	};
	public static final SampleProcessorFactory DEFAULT_PROCESSOR_FACTORY = new SampleProcessorFactory() {
		public SampleProcessor getInstance(SampleReader reader) {
			return new SampleProcessor(reader);
		}
	};
	
	public static final float HEARING_THRESHOLD = 1e-16f; // dynamic range between hearing threshold and instant perforation of eardrum
}
