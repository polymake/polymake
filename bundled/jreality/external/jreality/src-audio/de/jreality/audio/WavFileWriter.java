package de.jreality.audio;

import java.io.File;
import java.io.IOException;
import java.io.RandomAccessFile;

public class WavFileWriter {

    private int channels;
    private int sampleSize;
    private int sampleRate;
    
    private RandomAccessFile os;
	private boolean closed = false;
    private long dataSize = 0;

	/**
	 * @param channels   number of channels
	 * @param sampleRate sample rate
	 * @param sampleSize sample size in bits
	 * @param outFile    output file
	 * @throws IOException
	 */
    public WavFileWriter(int channels, int sampleRate, int sampleSize, File outFile) throws IOException {
    	os = new RandomAccessFile(outFile, "rw");
    	this.channels = channels;
    	this.sampleRate = sampleRate;
    	this.sampleSize = sampleSize;
    	os.seek(44); // keep the first 44 bits for the file header...
    	
    	// register shutdown hook which writes the file header and
    	// closes the file
    	Runtime.getRuntime().addShutdownHook(new Thread() {
    		@Override
    		public void run() {
    			try {
					close();
				} catch (IOException e) {
					e.printStackTrace();
				}
    		}
    	});
    }
    
    private void writeInt(int arg0) throws IOException {
        os.writeByte(arg0 & 0xff);
        os.writeByte((arg0 >> 8) & 0xff);
        os.writeByte((arg0 >> 16) & 0xff);
        os.writeByte((arg0 >> 24) & 0xff);
    }
    
    private void writeShort(int arg0) throws IOException {
    	os.writeByte(arg0 & 0xff);
    	os.writeByte((arg0 >> 8) & 0xff);
    }
    
    /**
     * @param buf    byte buffer containing little-endian PCM data; buffer contains one sample for each channel, followed by the next sample for each channel, etc.
     * @param offset
     * @param len
     * @throws IOException
     */
    public synchronized void write(byte[] buf, int offset, int len) throws IOException {
    	if (closed) return;
    	os.write(buf, offset, len);
    	dataSize+=len;
    }
    
    public synchronized void close() throws IOException {
    	// write file header
    	os.seek(0);
        os.write("RIFF".getBytes());
        writeInt((int) dataSize + 36);
        os.write("WAVEfmt ".getBytes());
        os.write(new byte[] {0x10, 0x00, 0x00, 0x00});
        os.write(new byte[] {0x01, 0x00});
        writeShort(channels);
        writeInt(sampleRate);
        writeInt(sampleRate * channels * ((sampleSize + 7) / 8));
        writeShort(channels * ((sampleSize + 7) / 8));
        writeShort(sampleSize);
        os.write("data".getBytes());
        writeInt((int) dataSize);

        // close file
    	os.close();
    	closed=true;
    }
}
