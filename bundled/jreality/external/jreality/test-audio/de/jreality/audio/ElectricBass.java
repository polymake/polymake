/*
 * Created on 17.10.2008
 *
 * This file is part of the de.jreality.audio package.
 * 
 * This program is free software; you can redistribute and/or modify 
 * it under the terms of the GNU General Public License as published 
 * by the Free Software Foundation; either version 2 of the license, or
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITTNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License 
 * along with this program; if not, write to the 
 * Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307
 * USA 
 */
package de.jreality.audio;

import java.text.NumberFormat;


public class ElectricBass extends RingBufferSource {

    public ElectricBass(String name) {
        super(name);
        numberFormat.setMaximumFractionDigits(3);
        numberFormat.setMinimumFractionDigits(3);
        for (int i = 0; i < this.programs.length; i++)
            this.programs[i] = new VB2Program();
        for (int i = 0; i < this.channelPrograms.length; i++)
            this.channelPrograms[i] = i;

        this.setProgram(0);
        sampleRate = 44100;
        this.setSampleRate(sampleRate);
        ringBuffer = new RingBuffer(sampleRate);
    }

    @Override
    protected void reset() {
        noteOff();

    }

    @Override
    protected void writeSamples(int n) {
        float[] buf = new float[n];
        processReplacing(buf, n);
        ringBuffer.write(buf, 0, n);
        hasChanged = true;
    }

    

    private static NumberFormat numberFormat = NumberFormat.getNumberInstance();
    public final static int PARAM_VOLUME1 = 0;
    public final static int PARAM_BALANCE = 1;
    public final static int PARAM_PUP_POS1 = 2;
    public final static int PARAM_PUP_POS2 = 3;

    public final static int PARAM_DAMPING = 4;

    public final static int PARAM_PLUG_POS = 5;

    public final static int PARAM_FINGER = 6;

    public final static int PARAM_SHAPE = 7;
    public final static int PARAM_SHAPE2 = 8;
    public final static int PARAM_Q = 9;

    public static final String PARAM_LABELS[] = { "", "", "", "", "", "","", "","","" };

    public static final String PARAM_NAMES[] = { "volume 1", "balance", "pup pos 1", "pup pos 2","damping",
            "pick pos", "finger", "shape", "lowpass", "Q" };

    public final static int NUM_PARAMS = PARAM_LABELS.length;

    private static final int NUM_PROGRAMS = 16;

    private static final int NUM_OUTPUTS = 1;

    private VB2Program[] programs = new VB2Program[NUM_PROGRAMS];

    private int channelPrograms[] = new int[NUM_PROGRAMS];

    private int currentProgram;

    private int noteOn = 0;

    private int currentNote;

    private float srate = 44100f;
    private double freq;

    private float volume1 = .8f;
    private float volume2 = .5f;

    private float pickPos = .2f;

    private float pupPos1 = .2f;
    private int puPos1;
    private float pupPos2 = .4f;
    private int puPos2;

    private float damping = .7f;
    private float alpha;
    private float shape = .7f;
    private float finger = .0f;
    private float shape2 = 1f;
    private float Q = .5f;

    // damps on noteoff;
    private float noteDamp = 0f;

    private float[] leftLine = new float[(int) (srate / 20)];

    private float[] rightLine = new float[(int) (srate / 20)];

    private int currLength = 1;
    private int bufferLength = 1;
    
    private double idealLength = 1;
    private float lengthFraction=0;

    private int writePos;

    private int readPos;
    private boolean soft = false;


    public void setSampleRate(float s) {
        srate = s;
        bufferLength = (int) (srate / 20);
        leftLine = new float[bufferLength];
        rightLine = new float[bufferLength];
        setf4();
    }

    public void setProgram(int index) {
        if (index < 0 || index >= NUM_PROGRAMS)
            return;
        currentProgram = index;
        this.programs[index].applyTo(this);
    }

    public void setProgramName(String name) {
        this.programs[this.currentProgram].setName(name);
    }

    public String getProgramName() {
        String name;

        if (programs[currentProgram].getName().equals("Init")) {
            name = programs[currentProgram].getName() + " "
                    + (currentProgram + 1);
        } else {
            name = programs[currentProgram].getName();
        }

        return name;
    }

    public String getParameterLabel(int index) {
        return PARAM_LABELS[index];
    }

    public String getParameterDisplay(int index) {
        return numberFormat.format(getParameter(index));
        //return Float.toString(getParameter(index));
    }

    public String getParameterName(int index) {
        return PARAM_NAMES[index];
    }

    public void setParameter(int index, float value) {

        switch (index) {
        case PARAM_VOLUME1: {
            setVolume1(value);
            return;
        }
        case PARAM_BALANCE: {
            setVolume2(value);
            return;
        }
        case PARAM_PUP_POS1: {
            setPupPos1(value);
            return;
        }
        case PARAM_PUP_POS2: {
            setPupPos2(value);
            return;
        }
        case PARAM_DAMPING: {
            setDamping(value);
            return;
        }
        case PARAM_PLUG_POS: {
            setPickPos(value);
            return;
        }
        case PARAM_FINGER: {
            setFinger(value);
            return;
        }
        case PARAM_SHAPE: {
            setShape(value);
            return;
        }
        case PARAM_SHAPE2: {
            setShape2(value);
            return;
        }
        case PARAM_Q: {
            setQ(value);
            return;
        }
        }

    }

    public float getParameter(int index) {
        switch (index) {
        case PARAM_VOLUME1:
            return getVolume1();
        case PARAM_BALANCE:
            return getVolume2();
        case PARAM_PUP_POS1:
            return getPupPos1();
        case PARAM_PUP_POS2:
            return getPupPos2();
        case PARAM_PLUG_POS:
            return getPickPos();
        case PARAM_DAMPING:
            return getDamping();
        case PARAM_FINGER:
            return getFinger();
        case PARAM_SHAPE:
            return getShape();
        case PARAM_SHAPE2:
            return getShape2();
        case PARAM_Q:
            return getQ();
        }
        return 0;
    }


    public String getProgramNameIndexed(int category, int index) {
        String text = "";
        if (index < this.programs.length)
            text = this.programs[index].getName();
        if ("Init".equals(text))
            text = text + " " + index;
        return text;
    }

    public boolean copyProgram(int destination) {
        if (destination < NUM_PROGRAMS) {
            this.programs[destination] = this.programs[this.currentProgram];
            return true;
        }
        return false;
    }

    public String getEffectName() {
        return "VB 2 v0.1a";
    }

    public String getVendorString() {
        return "http://www.randform.org";
    }

    public String getProductString() {
        return "VB-2a";
    }
    
    

    public int getVendorVersion() {
        return 0133;
    }

    public int getNumPrograms() {
        return NUM_PROGRAMS;
    }

    public int getNumParams() {
        return NUM_PARAMS;
    }

    public boolean setBypass(boolean value) {
        return false;
    }

    public int getProgram() {
        return this.currentProgram;
    }


    public boolean string2Parameter(int index, String value) {
        try {
            if (value != null)
                //this.setParameter(index, Float.parseFloat(value));
                setParameter(index, numberFormat.parse(value).floatValue());
            return true;
        } catch (Exception e) {
           e.printStackTrace();
        }

        return false;
    }

   

    

    /*
     * //DEPRECATED since 2.4 public void process(float[][] input, float[][]
     * output, int samples) { ...
     *  }
     */
    
    public final void processReplacing( final float[] out1,
            final int samples) {
        //float alpha = 1/((.05f+.95f*damping)*2*srate/10000f + 1);
        float alpha = 1/((.05f+.95f*(soft?1:damping))*2*srate/10000f + 1);
        if (noteDamp > 0) {
            for (int j = 0; j < samples; j++) {
                //
                // extra linear damping if note is off (-> release time)
                //
                if (noteOn==0) {
                    noteDamp -= 1f / (10f*currLength);
                }
                //
                // we average here to model fractional delay line lengths...
                //
                float leftNut = lengthFraction*leftLine[readPos] 
                              + (1-lengthFraction)*leftLine[(readPos + 1) % bufferLength];

                float rightBridge = lengthFraction* rightLine[readPos]
                                  + (1-lengthFraction)* rightLine[(readPos +1) % bufferLength];;
                
                                  
                writePos++;
                readPos++;
                
                if (writePos >= bufferLength)
                    writePos = 0;
                if (readPos >= bufferLength)
                    readPos = 0;
                
                rightLine[writePos] = -leftNut;
                leftLine[writePos] = -rightBridge;
                
                //
                // filtering
                //
                
                // 1st order iir filter
                // this is not(!) linear in phase (delay is frequency dependent) therefore we
                // compensate this in setf4() by changing the delay line length
                rightLine[writePos] = noteDamp * (0.999f-damping/100)*
                (       + (1-alpha)* rightLine[(writePos-1+bufferLength)%bufferLength] 
                        + alpha * rightLine[writePos]
                );
                leftLine[writePos] = noteDamp * (0.999f-damping/100)*
                (       + (1-alpha)* leftLine[(writePos-1+bufferLength)%bufferLength] 
                        + alpha * leftLine[writePos]
                );
                
                //
                // 2 pick ups. we read two neighbouring positions each so that we get the velocity
                // (not the position) as readout. shape adds nonlinearity by changing the signal amplitude 
                // with the distance itself. Finally a biquad lowpass filter is applied.
                //

                float dista1 = rightLine[(writePos - puPos1 + bufferLength) % bufferLength]
                                        + leftLine[(readPos + puPos1) % bufferLength];
                float distb1 = rightLine[(writePos - puPos1 + 1 + bufferLength)
                                        % bufferLength] + leftLine[(readPos + puPos1 + 1)
                                                                    % bufferLength];
                float dista2 = rightLine[(writePos - puPos2 + bufferLength) % bufferLength]
                                            + leftLine[(readPos + puPos2) % bufferLength];
                float distb2 = rightLine[(writePos - puPos2 + 1 + bufferLength)
                                         % bufferLength] + leftLine[(readPos + puPos2 + 1)
                                                                        % bufferLength];
                
                out1[j] = f3(currLength/srate*880*volume1*((dista1 - distb1) * volume2 * (shape +(1-shape)*(1+2f*(dista1+distb1))) +
                (dista2 - distb2) * (1-volume2) * (shape +(1-shape)*(1+2f*(dista2+distb2)))));
            }
        }

    }

    public int processEvents(byte[][] midiDataArray) {
        for (int i = 0; i < midiDataArray.length; i++) {
           
            byte[] midiData = midiDataArray[i];
            int status = midiData[0] & 0xf0;// ignoring channel

            if (status == 0x90 || status == 0x80) {
                // we only look at notes
                int note = midiData[1] & 0x7f;
                int velocity = midiData[2] & 0x7f;
                if (status == 0x80)
                    velocity = 0; // note off by velocity 0

                if (velocity == 0 /*&& (note == currentNote)*/)
                    this.noteOff();
                else
                    this.noteOn(note, velocity); 
                                                
            } else if (status == 0xb0) {
                // all notes off
                if (midiData[1] == 0x7e || midiData[1] == 0x7b) {
                    if(noteOn>1) noteOn = 1;
                    noteOff();
                    noteOn = 0;
                } else if ( midiData[1] == 67 ) { // 67 soft pedal
                    soft = midiData[2] > 64;
                    setf4();
                }
            } 
        }

        return 1; // want more
    }

    public void noteOn(int note, int vel) {
        currentNote = note;
        freq = (440f * (float) Math.pow(2,
                ((double) note - 57) * (1. / 12.)));
        idealLength = srate / freq ; //-2.5 
        
        //currLength = (int) (idealLength);
        //lengthFraction = (float)(idealLength - currLength);
        //currLength++;
        
        // calculate the filters phase drift and set currlength and lengthFraction;
        setf4();
        
        puPos1 = 3+(int) (pupPos1 * (currLength / 2f));
        puPos2 = 3+(int) (pupPos2 * (currLength / 2f));

        //
        // set the initial condition on both lines:
        //
        
        if (noteOn==0) {
            int pp = (int) (0.5 * pickPos * currLength);
            float vol = vel / 127f;
            readPos = 0;
            writePos = readPos+currLength-1;
            for (int i = 0; i < currLength; i++) {
                int lp = (i);
                int rp = (currLength - 1 - i);
//              leftLine[lp] = rightLine[rp] = vol
//                      * ((i < pp) ? i / (float) pp : (currLength - 1 - i)
//                              / (float) (currLength - 1 - pp));
                
                leftLine[lp] = rightLine[rp] = (float) (vol
                * cu( (i < pp) ? i / (float) pp : 
                    (currLength - 1 - i) / (float) (currLength - 1 - pp), finger*vol) );
            }
        }
        
        noteDamp = 1f;
        noteOn++;
    }
    
    private final float cu(final float x, final float f) {
        return x* (( 3-2* x)*x + f *(1-3*x+ 2*x*x));
    }

    public void noteOff() {
        if(noteOn>0)
            noteOn--;
    }

//  private void fillProgram(int channel, int prg, MidiProgramName mpn) {
//      if (channel == 9) {
//          // drums
//          mpn.setName("Standard");
//          mpn.setMidiProgram((byte) 0);
//          mpn.setParentCategoryIndex(0);
//      } else {
//          mpn.setName(GMNames.GM_NAMES[prg]);
//          mpn.setMidiProgram((byte) prg);
//          mpn.setParentCategoryIndex(-1); // for now
//
//          for (int i = 0; i < GMNames.NUM_GM_CATEGORIES; i++) {
//              if (prg >= GMNames.GM_CATEGORIES_FIRST_INDICES[i]
//                      && prg < GMNames.GM_CATEGORIES_FIRST_INDICES[i + 1]) {
//                  mpn.setParentCategoryIndex(i);
//                  break;
//              }
//          }
//
//      }
//
//  }

    public float getDamping() {
        return damping;
    }

    public void setDamping(float damping) {
        this.damping = damping;
        programs[currentProgram].setDamping(damping);
        setf4();
    }

    public float getPickPos() {
        return pickPos;
    }

    public void setPickPos(float pickPos) {
        this.pickPos = pickPos;
        programs[currentProgram].setPickPos(pickPos);
    }

    public float getPupPos1() {
        return pupPos1;
    }

    public void setPupPos1(float pupPos) {
        this.pupPos1 = pupPos;
        programs[currentProgram].setPupPos1(pupPos);
    }
    public float getPupPos2() {
        return pupPos2;
    }

    public void setPupPos2(float pupPos) {
        this.pupPos2 = pupPos;
        programs[currentProgram].setPupPos2(pupPos);
    }

    void setVolume1(float value) {
        volume1 = value;
        programs[currentProgram].setVolume1(value);
    }

    private float getVolume1() {
        return volume1;
    }
    
    void setVolume2(float value) {
        volume2 = value;
        programs[currentProgram].setVolume2(value);
    }

    private float getVolume2() {
        return volume2;
    }

    public float getShape() {
        return shape;
    }

    public void setShape(float shape) {
        this.shape = shape;
        programs[currentProgram].setShape(shape);
    }

    public float getFinger() {
        return finger;
    }

    public void setFinger(float finger) {
        this.finger = finger;
        programs[currentProgram].setFinger(finger);
    }
    public float getQ() {
        return Q;
    }

    public void setQ(float q) {
        Q = q;
        programs[currentProgram].setQ(q);
        setf3(2000*shape2,0.1f+2*Q);
    }

    public float getShape2() {
        return shape2;
    }

    public void setShape2(float shape2) {
        this.shape2 = shape2;
        programs[currentProgram].setShape2(shape2);
        setf3(2000*shape2,0.1f+2*Q);
    }

    //
    //   filters
    //
    
    // input averaging
    private float f1state;
    private float f1(final float x ) {
        float y = (1-alpha)* f1state + alpha*x;
        f1state = x;
        return y;
    }
    // recursive averaging
    private float f2state;
    private float f2(final float x ) {
        float y = (1-alpha)* f2state + alpha*x;
        f2state = y;
        return y;
    }
    
    
    // biquad lpf 
    private float f3statex1, f3statex2, f3statey1, f3statey2;
    private float a0,a1,a2,b0,b1,b2;
    private void setf3(float freq, float Q) {
        
        double w0 = 2*Math.PI*(freq)/srate;
        float cosw0 = (float) Math.cos(w0);
        float f3alpha = (float) (Math.sin(w0)/(2*Q));
        a0 =   1 + f3alpha;
        b0 =  ((1 - cosw0)/2)/a0;
        b1 =   (1 - cosw0)/a0;
        b2 =  ((1 - cosw0)/2)/a0;
        a1 =  (-2*cosw0)/a0;
        a2 =   (1 - f3alpha)/a0;
    }
    private float f3(final float x)  {
        float y = b0*x + b1*f3statex1 + b2*f3statex2 - a1*f3statey1 - a2*f3statey2;
        f3statey2 = f3statey1;
        f3statey1 = y;
        f3statex2 = f3statex1;
        f3statex1 = x;
        
        return y;
    }

    
    
    private void setf4() {
        alpha = 1/((.05f+.95f*(soft?1:damping))*2*srate/10000f + 1);
        //alpha = 1/((.05f+.95f*(damping))*2*srate/10000f + 1);
        double w = 2*Math.PI/srate * freq;
        double cosw = Math.cos(w);
        double sinw = Math.sin(w);
        double reh = (alpha*(1 + (-1 + alpha)*cosw))/
           (2 - 2*alpha + alpha*alpha + 2*(-1 + alpha)*cosw);
        double imh = -(((-1 + alpha)*alpha*sinw)/
                 (2 - 2*alpha + alpha*alpha + 
                           2*(-1 + alpha)*cosw));
        double comp =  (Math.atan2(imh, reh)/(2*Math.PI)*srate/freq);
        currLength = (int) (idealLength - comp);
        lengthFraction =  (float)(idealLength - comp - currLength);
        //writePos = (readPos+currLength)%bufferLength;
        readPos = (writePos-currLength+bufferLength)%bufferLength;
        // currLength gets an extra sample for the fractional interpolation...
        currLength++;
    }
    
    
}

// to store the presets:

 class VB2Program {

    private String name = "Init";

    private float volume1 = .8f;

    private float pupPos1 = 0.4f;
    
    private float volume2 = .5f;

    private float pupPos2 = 0.4f;
    
    private float pickPos = .2f;

    private float damping = 0.7f;

    private float shape = 0.7f;
    
    private float finger = 0f;
    
    private float shape2 = 1f;

    private float q = 0.5f;

    public void applyTo(ElectricBass vb2) {
        vb2.setVolume1(volume1);
        vb2.setPupPos1(pupPos1);
        vb2.setVolume2(volume2);
        vb2.setPupPos2(pupPos2);
        vb2.setDamping(damping);
        vb2.setPickPos(pickPos);
        vb2.setShape(shape);
        vb2.setFinger(finger);
        vb2.setShape2(shape2);
        vb2.setQ(q);
    }

    public String getName() {
        return name;
    }

    public void setName(String name) {
        this.name = name;
    }

    public float getVolume1() {
        return volume1;
    }

    public void setVolume1(float v) {
        this.volume1 = v;
    }

    public float getPupPos1() {
        return pupPos1;
    }
    
    public void setPupPos1(float pupPos) {
        this.pupPos1 = pupPos;
    }
    
    public float getVolume2() {
        return volume2;
    }

    public void setVolume2(float v) {
        this.volume2 = v;
    }

    public float getPupPos2() {
        return pupPos2;
    }
    
    public void setPupPos2(float pupPos) {
        this.pupPos2 = pupPos;
    }

    public float getDamping() {
        return damping;
    }

    public void setDamping(float damping) {
        this.damping = damping;
    }

    public float getPickPos() {
        return pickPos;
    }

    public void setPickPos(float pickPos) {
        this.pickPos = pickPos;
    }


    public float getShape() {
        return shape;
    }

    public void setShape(float shape) {
        this.shape = shape;
    }

    public float getFinger() {
        return finger;
    }

    public void setFinger(float finger) {
        this.finger = finger;
    }

    public float getQ() {
        return q;
    }

    public void setQ(float q) {
        this.q = q;
    }

    public float getShape2() {
        return shape2;
    }

    public void setShape2(float shape2) {
        this.shape2 = shape2;
    }
    
}
