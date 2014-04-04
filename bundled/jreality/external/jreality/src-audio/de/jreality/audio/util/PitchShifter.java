package de.jreality.audio.util;

import java.util.Arrays;

/*
 *
 * NAME: smbPitchShift.cpp
 * VERSION: 1.2
 * HOME URL: http://www.dspdimension.com
 * KNOWN BUGS: none
 *
 *
 * COPYRIGHT 1999-2006 Stephan M. Bernsee <smb [AT] dspdimension [DOT] com>
 *
 * 						The Wide Open License (WOL)
 *
 * Permission to use, copy, modify, distribute and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice and this license appear in all source copies. 
 * THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT EXPRESS OR IMPLIED WARRANTY OF
 * ANY KIND. See http://www.dspguru.com/wol.htm for more information.
 *
 */

/**
 * Class for doing pitch shifting while maintaining duration using the Short Time Fourier Transform.
 *
 * @author Stephan M. Bernsee (original cpp version 1.2 smbPitchShift.cpp, see <a href="http://www.dspguru.com">http://www.dspguru.com/wol.htm</a>)
 * @author Steffen Weissmann (java port)
 */
public class PitchShifter {

	private static final double M_PI = Math.PI;
	float[] gInFIFO;
	float[] gOutFIFO;
	float[] gFFTworksp;
	float[] gLastPhase;
	float[] gSumPhase;
	float[] gOutputAccum;
	float[] gAnaFreq;
	float[] gAnaMagn;
	float[] gSynFreq;
	float[] gSynMagn;

	private int MAX_FRAME_LENGTH;

	public PitchShifter(int maxFrameSize) {
		this.MAX_FRAME_LENGTH=maxFrameSize;
		gInFIFO=new float[MAX_FRAME_LENGTH];
		gOutFIFO=new float[MAX_FRAME_LENGTH];
		gFFTworksp=new float[2*MAX_FRAME_LENGTH];
		gLastPhase=new float[MAX_FRAME_LENGTH/2+1];
		gSumPhase=new float[MAX_FRAME_LENGTH/2+1];
		gOutputAccum=new float[2*MAX_FRAME_LENGTH];
		gAnaFreq=new float[MAX_FRAME_LENGTH];
		gAnaMagn=new float[MAX_FRAME_LENGTH];
		gSynFreq=new float[MAX_FRAME_LENGTH];
		gSynMagn=new float[MAX_FRAME_LENGTH];
	}

	private float sampleRate=44100;
	private int fftFrameSize=1024;
	private int oversampling=32;
	private float pitchShift=1f;

	public int getMaxFrameLength() {
		return MAX_FRAME_LENGTH;
	}

	public int getSampleRate() {
		return (int) sampleRate;
	}

	public synchronized void setSampleRate(int sampleRate) {
		this.sampleRate = sampleRate;
	}

	public int getFftFrameSize() {
		return fftFrameSize;
	}

	/**
	 * fftFrameSize defines the FFT frame size used for the processing. Typical values are 1024, 2048 and 4096. It may
	 * be any value <= maxFrameLength but it MUST be a power of 2.
	 * @param fftFrameSize
	 */
	public synchronized void setFftFrameSize(int fftFrameSize) {
		this.fftFrameSize = fftFrameSize;
	}

	public int getOversampling() {
		return oversampling;
	}

	/**
	 * This is the STFT oversampling factor which also determines the overlap between
	 * adjacent STFT frames. It should at least be 4 for moderate scaling ratios.
	 * A value of 32 is recommended for best quality; for real time efficiency use smaller values.
	 * 
	 * @param oversampling the oversampling factor
	 */
	public synchronized void setOversampling(int oversampling) {
		this.oversampling = oversampling;
	}

	public double getPitchShift() {
		return pitchShift;
	}

	/**
	 * The pitchShift factor is a value between 0.5 (one octave down) and 2. (one octave up).
	 * A value of exactly 1 does not change the pitch.
	 * 
	 * @param pitchShift the new pitch shift factor
	 */
	public void setPitchShift(double pitchShift) {
		this.pitchShift = (float) pitchShift;
	}


	// -----------------------------------------------------------------------------------------------------------------


	/**
	 * Pitch shifts the data from indata and writes it to outdata.
	 * The two buffers can be identical (ie. it can process the data in-place).
	 * 
	 * The data passed to the routine in indata should be in the range [-1.0, 1.0),
	 * which is also the output range for the data, make sure you scale the
	 * data accordingly (for 16bit signed integers you would have to divide
	 * (and multiply) by 32768). 
	 * 
	 * @param indata the input data buffer
	 * @param outdata the output data buffer
	 * @param numSampsToProcess tells the routine how many samples to process.
	 * @param offset offset for both indata and outdata
	 */
	public synchronized void smbPitchShift(float[] indata, float[] outdata, int offset, int numSampsToProcess)
	/*
		Routine smbPitchShift(). See top of file for explanation
		Purpose: doing pitch shifting while maintaining duration using the Short
		Time Fourier Transform.
		Author: (c)1999-2006 Stephan M. Bernsee <smb [AT] dspdimension [DOT] com>
	 */
	{

		int gRover = 0;
		double magn, phase, tmp, window, real, imag;
		double freqPerBin, expct;
		int i,k, qpd, index, inFifoLatency, stepSize, fftFrameSize2;

		/* set up some handy variables */
		fftFrameSize2 = fftFrameSize/2;
		stepSize = fftFrameSize/oversampling;
		freqPerBin = sampleRate/(double)fftFrameSize;
		expct = 2.*M_PI*(double)stepSize/(double)fftFrameSize;
		inFifoLatency = fftFrameSize-stepSize;
		if (gRover == 0) gRover = inFifoLatency;

		/* main processing loop */
		for (i = 0; i < numSampsToProcess; i++){

			/* As long as we have not yet collected enough data just read in */
			gInFIFO[gRover] = indata[i+offset];
			outdata[i+offset] = gOutFIFO[gRover-inFifoLatency];
			gRover++;

			/* now we have enough data for processing */
			if (gRover >= fftFrameSize) {
				gRover = inFifoLatency;

				/* do windowing and re,im interleave */
				for (k = 0; k < fftFrameSize;k++) {
					window = -.5*Math.cos(2.*M_PI*(double)k/(double)fftFrameSize)+.5;
					gFFTworksp[2*k] = (float) (gInFIFO[k] * window);
					gFFTworksp[2*k+1] = 0.f;
				}


				/* ***************** ANALYSIS ******************* */
				/* do transform */
				smbFft(gFFTworksp, fftFrameSize, -1);

				/* this is the analysis step */
				for (k = 0; k <= fftFrameSize2; k++) {

					/* de-interlace FFT buffer */
					real = gFFTworksp[2*k];
					imag = gFFTworksp[2*k+1];

					/* compute magnitude and phase */
					magn = 2.*Math.sqrt(real*real + imag*imag);
					phase = Math.atan2(imag,real);

					/* compute phase difference */
					tmp = phase - gLastPhase[k];
					gLastPhase[k] = (float) phase;

					/* subtract expected phase difference */
					tmp -= (double)k*expct;

					/* map delta phase into +/- Pi interval */
					qpd = (int) (tmp/M_PI);
					if (qpd >= 0) qpd += qpd&1;
					else qpd -= qpd&1;
					tmp -= M_PI*(double)qpd;

					/* get deviation from bin frequency from the +/- Pi interval */
					tmp = oversampling*tmp/(2.*M_PI);

					/* compute the k-th partials' true frequency */
					tmp = (double)k*freqPerBin + tmp*freqPerBin;

					/* store magnitude and true frequency in analysis arrays */
					gAnaMagn[k] = (float) magn;
					gAnaFreq[k] = (float) tmp;

				}

				/* ***************** PROCESSING ******************* */
				/* this does the actual pitch shifting */
				//				memset(gSynMagn, 0, fftFrameSize*sizeof(float));
				//				memset(gSynFreq, 0, fftFrameSize*sizeof(float));

				Arrays.fill(gSynMagn, 0);
				//Arrays.fill(gSynFreq, 0);

				for (k = 0; k <= fftFrameSize2; k++) { 
					index = (int) (k*pitchShift);
					if (index <= fftFrameSize2) { 
						gSynMagn[index] += gAnaMagn[k]; 
						gSynFreq[index] = gAnaFreq[k] * pitchShift; 
					} 
				}

				/* ***************** SYNTHESIS ******************* */
				/* this is the synthesis step */
				for (k = 0; k <= fftFrameSize2; k++) {

					/* get magnitude and true frequency from synthesis arrays */
					magn = gSynMagn[k];
					tmp = gSynFreq[k];

					/* subtract bin mid frequency */
					tmp -= (double)k*freqPerBin;

					/* get bin deviation from freq deviation */
					tmp /= freqPerBin;

					/* take osamp into account */
					tmp = 2.*M_PI*tmp/oversampling;

					/* add the overlap phase advance back in */
					tmp += (double)k*expct;

					/* accumulate delta phase to get bin phase */
					gSumPhase[k] += tmp;
					phase = gSumPhase[k];

					/* get real and imag part and re-interleave */
					gFFTworksp[2*k] = (float) (magn*Math.cos(phase));
					gFFTworksp[2*k+1] = (float) (magn*Math.sin(phase));
				} 

				/* zero negative frequencies */
				for (k = fftFrameSize+2; k < 2*fftFrameSize; k++) gFFTworksp[k] = 0.f;

				/* do inverse transform */
				smbFft(gFFTworksp, fftFrameSize, 1);

				/* do windowing and add to output accumulator */ 
				for(k=0; k < fftFrameSize; k++) {
					window = -.5*Math.cos(2.*M_PI*(double)k/(double)fftFrameSize)+.5;
					gOutputAccum[k] += 2.*window*gFFTworksp[2*k]/(fftFrameSize2*oversampling);
				}
				for (k = 0; k < stepSize; k++) gOutFIFO[k] = gOutputAccum[k];

				/* shift accumulator */
				//memmove(gOutputAccum, gOutputAccum+stepSize, fftFrameSize*sizeof(float));
				System.arraycopy(gOutputAccum, stepSize, gOutputAccum, 0, fftFrameSize);

				/* move input FIFO */
				for (k = 0; k < inFifoLatency; k++) gInFIFO[k] = gInFIFO[k+stepSize];
			}
		}
	}

	// -----------------------------------------------------------------------------------------------------------------


	void smbFft(float[] fftBuffer, int fftFrameSize, int sign)
	/* 
		FFT routine, (C)1996 S.M.Bernsee. Sign = -1 is FFT, 1 is iFFT (inverse)
		Fills fftBuffer[0...2*fftFrameSize-1] with the Fourier transform of the
		time domain data in fftBuffer[0...2*fftFrameSize-1]. The FFT array takes
		and returns the cosine and sine parts in an interleaved manner, ie.
		fftBuffer[0] = cosPart[0], fftBuffer[1] = sinPart[0], asf. fftFrameSize
		must be a power of 2. It expects a complex input signal (see footnote 2),
		ie. when working with 'common' audio signals our input signal has to be
		passed as {in[0],0.,in[1],0.,in[2],0.,...} asf. In that case, the transform
		of the frequencies of interest is in fftBuffer[0...fftFrameSize].
	 */
	{
		float wr, wi, arg, temp;
		float tr, ti, ur, ui;
		int i, bitm, j, le, le2, k;

		int p1, p2, p1r, p1i, p2r, p2i;

		for (i = 2; i < 2*fftFrameSize-2; i += 2) {
			for (bitm = 2, j = 0; bitm < 2*fftFrameSize; bitm <<= 1) {
				if ((i & bitm)!=0) j++;
				j <<= 1;
			}
			if (i < j) {
				//p1 = fftBuffer+i;
				p1=i;
				//p2 = fftBuffer+j;
				p2=j;
				//temp = *p1;
				temp = fftBuffer[p1];
				//*(p1++) = *p2;
				fftBuffer[p1++]=fftBuffer[p2];
				//*(p2++) = temp;
				fftBuffer[p2++] = temp;
				//temp = *p1;
				temp = fftBuffer[p1];
				//*p1 = *p2;
				fftBuffer[p1]=fftBuffer[p2];
				//*p2 = temp;
				fftBuffer[p2]=temp;
			}
		}
		for (k = 0, le = 2; k < (long)(Math.log(fftFrameSize)/Math.log(2.)+.5); k++) {
			le <<= 1;
			le2 = le>>1;
			ur = 1.0f;
			ui = 0.0f;
			arg = (float) (M_PI / (le2>>1));
			wr = (float) Math.cos(arg);
			wi = (float) (sign*Math.sin(arg));
			for (j = 0; j < le2; j += 2) {
				//p1r = fftBuffer+j;
				p1r = j;
				p1i = p1r+1;
				p2r = p1r+le2;
				p2i = p2r+1;
				for (i = j; i < 2*fftFrameSize; i += le) {
					tr = fftBuffer[p2r] * ur - fftBuffer[p2i] * ui;
					ti = fftBuffer[p2r] * ui + fftBuffer[p2i] * ur;
					fftBuffer[p2r] = fftBuffer[p1r] - tr;
					fftBuffer[p2i] = fftBuffer[p1i] - ti;
					fftBuffer[p1r] += tr;
					fftBuffer[p1i] += ti;
					p1r += le; p1i += le;
					p2r += le; p2i += le;
				}
				tr = ur*wr - ui*wi;
				ui = ur*wi + ui*wr;
				ur = tr;
			}
		}
	}


	// -----------------------------------------------------------------------------------------------------------------

	/*

	    12/12/02, smb

	    PLEASE NOTE:

	    There have been some reports on domain errors when the atan2() function was used
	    as in the above code. Usually, a domain error should not interrupt the program flow
	    (maybe except in Debug mode) but rather be handled "silently" and a global variable
	    should be set according to this error. However, on some occasions people ran into
	    this kind of scenario, so a replacement atan2() function is provided here.

	    If you are experiencing domain errors and your program stops, simply replace all
	    instances of atan2() with calls to the smbAtan2() function below.

	 */


	double smbAtan2(double x, double y)
	{
		double signx;
		if (x > 0.) signx = 1.;  
		else signx = -1.;

		if (x == 0.) return 0.;
		if (y == 0.) return signx * M_PI / 2.;

		return Math.atan2(x, y);
	}


	// -----------------------------------------------------------------------------------------------------------------
	// -----------------------------------------------------------------------------------------------------------------
	// -----------------------------------------------------------------------------------------------------------------

}
