package de.jreality.audio.javasound;

import java.io.IOException;
import java.util.LinkedList;
import java.util.List;

import javax.sound.sampled.AudioFormat;
import javax.sound.sampled.AudioFormat.Encoding;
import javax.sound.sampled.AudioInputStream;
import javax.sound.sampled.AudioSystem;
import javax.sound.sampled.DataLine;
import javax.sound.sampled.LineUnavailableException;
import javax.sound.sampled.Mixer;
import javax.sound.sampled.Mixer.Info;
import javax.sound.sampled.SourceDataLine;
import javax.sound.sampled.UnsupportedAudioFileException;
import javax.swing.JOptionPane;

import de.jreality.audio.AudioBackend;
import de.jreality.audio.SoundEncoder;
import de.jreality.util.Input;

public class JavaSoundUtility {

	public static boolean
		chooseFirstMixer = true;
	
	public static boolean BIG_ENDIAN = false;
	public static int BITS_PER_SAMPLE = 16;
	private static Mixer CURRENT_MIXER;
	
	private static int sampleRate = 44100;
	

	private JavaSoundUtility() {}

	public static int getSampleRate() {
		return sampleRate;
	}

	public static void setSampleRate(int sampleRate) {
		JavaSoundUtility.sampleRate = sampleRate;
	}

	public static boolean supportsChannels(int n) {
		AudioFormat f = outputFormat(n);
		SourceDataLine sdl = createSourceDataLine(f);
		return sdl != null;
	}
	
	public static SourceDataLine createSourceDataLine(AudioFormat audioFormat) {

		if (CURRENT_MIXER != null) {
			CURRENT_MIXER.close();
			CURRENT_MIXER = null;
		}

		DataLine.Info dataLineInfo = new DataLine.Info(SourceDataLine.class, audioFormat);
		
		Info[] mixerInfos = AudioSystem.getMixerInfo();
		List<Mixer> possibleMixers = new LinkedList<Mixer>();
		List<SourceDataLine> mixerLines = new LinkedList<SourceDataLine>();
		
		for (Info info : mixerInfos) {
			//System.out.println("Checking mixer "+info);
			Mixer mixer = AudioSystem.getMixer(info);
			try {
				mixer.open();
			} catch (LineUnavailableException e) {
				mixer.close();
				continue;
			}
	
			if (!mixer.isLineSupported(dataLineInfo)) {
				mixer.close();
				continue;
			}
				
			try {
				SourceDataLine ret = (SourceDataLine) mixer.getLine(dataLineInfo);
				possibleMixers.add(mixer);
				mixerLines.add(ret);
			} catch (LineUnavailableException e) {
				continue;
			}
		}
		
		if (possibleMixers.isEmpty()) {
			System.out.println("JavaSoundUtility.createSourceDataLine(): NO MIXER");
			return null;
		}
		if (possibleMixers.size() == 1) {
			//System.out.println("JavaSoundUtility.createSourceDataLine(): ONE MIXER");
			return mixerLines.get(0);
		}
		
		Info[] possibleInfos = new Info[possibleMixers.size()];
		for (int i=0; i<possibleMixers.size(); i++) {
			possibleInfos[i]=possibleMixers.get(i).getMixerInfo();
		}
		
		Info selectedInfo = null;
		if (chooseFirstMixer) {
			selectedInfo = possibleInfos[0];
		} else {
			selectedInfo = (Info) JOptionPane.showInputDialog(null, "Select audio device", "JavaSound", JOptionPane.OK_OPTION, null, possibleInfos, null);
		}
		
		int idx;
		for (idx=0; idx<possibleInfos.length; idx++) {
			if (possibleInfos[idx]==selectedInfo) break;
		}
		
		SourceDataLine selectedLine=mixerLines.get(idx);
		possibleMixers.remove(idx);

		for (Mixer m : possibleMixers) m.close();
		
		return selectedLine;
	}

	public static AudioFormat outputFormat(int channels) {
		return new AudioFormat(
				getSampleRate(), // the number of samples per second
				BITS_PER_SAMPLE, // the number of bits in each sample
				channels, // the number of channels
				true, // signed/unsigned PCM
				BIG_ENDIAN); // big endian ?
	}

	/**
	 * Convert float array to byte array.
	 * 
	 * @param byteSound
	 *            User provided byte array to return result in.
	 * @param dbuf
	 *            User provided float array to convert.
	 */
	public static final void floatToByte(byte[] byteSound, float[] dbuf) {
		floatToByte(byteSound, dbuf, BIG_ENDIAN);
	}

	public static final void floatToByte(byte[] byteSound, float[] dbuf, boolean bigEndian) {
		int bufsz = dbuf.length;
		int ib = 0;
		if (bigEndian) {
			for (int i = 0; i < bufsz; i++) {
				short y = (short) (32767. * dbuf[i]);
				byteSound[ib] = (byte) (y >> 8);
				ib++;
				byteSound[ib] = (byte) (y & 0x00ff);
				ib++;
			}
		} else {
			for (int i = 0; i < bufsz; i++) {
				short y = (short) (32767. * dbuf[i]);
				byteSound[ib] = (byte) (y & 0x00ff);
				ib++;
				byteSound[ib] = (byte) (y >> 8);
				ib++;
			}
		}
	}

	public static float[] readAudioFile(Input input) throws UnsupportedAudioFileException, IOException {
		AudioInputStream ain = AudioSystem.getAudioInputStream(input.getInputStream());
		return readAudioFile(ain);
	}
	
	public static float[] readAudioFile(AudioInputStream ain) {
		AudioFormat af = ain.getFormat();
		if (af.getEncoding() != Encoding.PCM_SIGNED) {
			System.out.println("converting from format "+af);
			AudioFormat baseFormat = af;
			float inSampleRate = baseFormat.getSampleRate();
			if (inSampleRate == AudioSystem.NOT_SPECIFIED) {
				inSampleRate = 44100;
			}
			af =
			    new AudioFormat(AudioFormat.Encoding.PCM_SIGNED,
			                    inSampleRate,
			                    16,
			                    baseFormat.getChannels(),
			                    baseFormat.getChannels() * 2,
			                    inSampleRate,
			                    false);
			System.out.println("to format "+af);
			ain = AudioSystem.getAudioInputStream(af, ain);
		}
		
		int bytesPerSample = af.getSampleSizeInBits()/8;

		int nSamples = (int) ain.getFrameLength();

		float[] samples = new float[nSamples];
		
		byte[] buf = new byte[1024*af.getFrameSize()];

		int read = -1;
		int idx=0;
		try {
			while ((read=ain.read(buf))!= -1) {
				int readFrames = read/af.getFrameSize();
				for (int i=0; i<readFrames; i++) {
					float v = getFloat(buf, i*af.getFrameSize(), bytesPerSample, false);
					samples[idx++]=v;
				}
			}
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		
		return samples;
		
	}

	public static float getFloat(byte[] b, int offset, int bytes /*per sample*/, boolean bigEndian) {
		float sample = 0;
		int ret = 0;
		int length = bytes;
		for (int i = 0; i < bytes; i++, length--)
		{
			ret |= ((int) (b[offset + i] & 0xFF) << ((((bigEndian) ? length : (i + 1)) * 8) - 8));
		}
		switch (bytes)
		{
		case 1 :
			if (ret > 0x7F)
			{
				ret = ~ret + 1;
				ret &= 0x7F;
				ret = ~ret + 1;
			}
			sample = (float) ((double) ret / (double) Byte.MAX_VALUE);
			break;
		case 2 :
			if (ret > 0x7FFF)
			{
				ret = ~ret + 1;
				ret &= 0x7FFF;
				ret = ~ret + 1;
			}
			sample = (float) ((double) ret / (double) Short.MAX_VALUE);
			break;
		case 3 :
			if (ret > 0x7FFFFF)
			{
				ret = ~ret + 1;
				ret &= 0x7FFFFF;
				ret = ~ret + 1;
			}
			sample = (float) ((double) ret / 8388608f);
			break;
		case 4 :
			sample = (float) ((double) ret / (double) Integer.MAX_VALUE);
			break;
		}
		return sample;
	}
	
	public static void launchAudioThread(final AudioBackend backend, final SoundEncoder enc, final int framesize, String label) {
		Runnable soundRenderer = new Runnable() {
			public void run() {
				while (!Thread.interrupted()) {
					backend.processFrame(enc, framesize);
				}
			}
		};

		final Thread soundThread = new Thread(soundRenderer);
		soundThread.setName(label);
		soundThread.setPriority(Thread.MAX_PRIORITY);
		soundThread.setDaemon(true);
		soundThread.start();
	}
}
