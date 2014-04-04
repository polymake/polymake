package de.jreality.sunflow.core.camera;

import org.sunflow.SunflowAPI;
import org.sunflow.core.CameraLens;
import org.sunflow.core.ParameterList;
import org.sunflow.core.Ray;

public class TiledPinholeLens implements CameraLens {
    private float au, av;
    private float aspect, fov;

    private int tilesX=1, tilesY=1, tileX=0, tileY=0;
    
    public TiledPinholeLens() {
        fov = 90;
        aspect = 1;
        update();
    }

    public boolean update(ParameterList pl, SunflowAPI api) {
        // get parameters
        fov = pl.getFloat("fov", fov);
        aspect = pl.getFloat("aspect", aspect);
        tilesX = pl.getInt("tilesX", 1);
        tilesY = pl.getInt("tilesY", 1);
        tileX = pl.getInt("tileX", 0);
        tileY = pl.getInt("tileY", 0);        
        update();
        return true;
    }

    private void update() {
        au = (float) Math.tan(Math.toRadians(fov * 0.5f));
        av = au / aspect;
    }

    public Ray getRay(float x, float y, int imageX, int imageY, double lensX, double lensY, double time) {
    	int imageWidth = imageX*tilesX;
    	int imageHeight = imageY*tilesY;
        float du = -au + ((2.0f * au * (x+tileX*imageX) ) / (imageWidth - 1.0f));
        float dv = -av + ((2.0f * av * (y+tileY*imageY) ) / (imageHeight - 1.0f));
        return new Ray(0, 0, 0, du, dv, -1);
    }
}