package de.jreality.audio;

import java.util.Arrays;

import javax.sound.sampled.AudioFormat;
import javax.sound.sampled.AudioSystem;
import javax.sound.sampled.DataLine;
import javax.sound.sampled.LineUnavailableException;
import javax.sound.sampled.Mixer;
import javax.sound.sampled.Mixer.Info;
import javax.sound.sampled.SourceDataLine;
import javax.swing.JFrame;

import de.jreality.audio.javasound.JavaSoundUtility;
import de.jreality.scene.AudioSource;
import de.jreality.scene.data.SampleReader;
import de.jtem.beans.InspectorPanel;

public class Test51 {

	private static final float SAMPLE_RATE = 44100;
	private static final boolean BIG_ENDIAN = false;
	SourceDataLine stereoOut;
	byte[] buffer;
	float[] fbuffer;
	private int framesize;
	private int byteLen;
	private int channels=5;
	private int channel;
	
	public Test51() throws LineUnavailableException {
		this.framesize = 1024;
		byteLen = framesize * channels * 2; // channels * 2 bytes per sample
		buffer = new byte[byteLen];
		fbuffer = new float[channels*framesize];
		
		Info[] mixerInfos = AudioSystem.getMixerInfo();
		System.out.println(Arrays.toString(mixerInfos));
		Info info = mixerInfos[0];
		Mixer mixer = AudioSystem.getMixer(info);
		mixer.open();

		AudioFormat audioFormat = new AudioFormat(
					SAMPLE_RATE, // the number of samples per second
					16, // the number of bits in each sample
					channels, // the number of channels
					true, // signed/unsigned PCM
					BIG_ENDIAN); // big endian ?
		
		DataLine.Info dataLineInfo = new DataLine.Info(SourceDataLine.class, audioFormat);
		if (!mixer.isLineSupported(dataLineInfo)) {
			throw new RuntimeException("no source data line found.");
		}
	
		stereoOut = (SourceDataLine) mixer.getLine(dataLineInfo);

		stereoOut.open(audioFormat, channels*byteLen);
		System.out.println("stereoOut bufferSize="+stereoOut.getBufferSize());
		stereoOut.start();
	}
	
	public void render(float[] monoSamples) {
		Arrays.fill(fbuffer, 0f);
		for (int i=0; i<framesize; i++) {
			fbuffer[channels*i+channel]=monoSamples[i];
		}
		JavaSoundUtility.floatToByte(buffer, fbuffer, BIG_ENDIAN);
		stereoOut.write(buffer, 0, byteLen);
	}
	
	public void setChannel(int ch) {
		this.channel=ch%channels;
	}
	
	public int getChannel() {
		return channel;
	}
	
	public static void main(String[] args) throws Exception {
		Test51 t = new Test51();
		AudioSource src = new SynthSource("foo", 44100) {
			double frequency = 660;
			@Override
			public float nextSample() {
				return (float) Math.sin(2*Math.PI*frequency*index/sampleRate);
			}
		};
		
		src.start();
		
		InspectorPanel ip = new InspectorPanel();
		ip.setObject(t);
		JFrame f = new JFrame("5.1 Test");
		f.getContentPane().add(ip);
		f.pack();
		f.setVisible(true);
		
		float[] fbuf = new float[1024];
		
		SampleReader sr = ConvertingReader.createReader(src.createReader(), 44100, AudioAttributes.DEFAULT_INTERPOLATION_FACTORY);
		
		while (true) {
			sr.read(fbuf, 0, 1024);
			t.render(fbuf);
		}
		
	}
}
