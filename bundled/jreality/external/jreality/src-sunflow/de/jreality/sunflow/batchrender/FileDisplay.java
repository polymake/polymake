package de.jreality.sunflow.batchrender;

import org.sunflow.core.Display;
import org.sunflow.image.Bitmap;
import org.sunflow.image.Color;

public class FileDisplay implements Display {
    private Bitmap bitmap;
    private String filename;

    public FileDisplay(boolean saveImage) {
        // a constructor that allows the image to not be saved
        // usefull for benchmarking purposes
        bitmap = null;
        filename = saveImage ? "output.png" : null;
    }

    public FileDisplay(String filename) {
        bitmap = null;
        this.filename = filename == null ? "output.png" : filename;
    }

    public void imageBegin(int w, int h, int bucketSize) {
        if (bitmap == null || bitmap.getWidth() != w || bitmap.getHeight() != h)
            bitmap = new Bitmap(w, h, filename == null || filename.endsWith(".hdr"));
    }

    public void imagePrepare(int x, int y, int w, int h, int id) {
    }

    public void imageUpdate(int x, int y, int w, int h, Color[] data) {
        for (int j = 0, index = 0; j < h; j++)
            for (int i = 0; i < w; i++, index++)
                bitmap.setPixel(x + i, bitmap.getHeight() - 1 - (y + j), data[index]);
    }

    public void imageFill(int x, int y, int w, int h, Color c) {
        Color cg = c;
        for (int j = 0; j < h; j++)
            for (int i = 0; i < w; i++)
                bitmap.setPixel(x + i, bitmap.getHeight() - 1 - (y + j), cg);
    }

    public void imageEnd() {
        if (filename != null)
            bitmap.save(filename);
    }
}