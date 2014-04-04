/**
 *
 * This file is part of jReality. jReality is open source software, made
 * available under a BSD license:
 *
 * Copyright (c) 2003-2006, jReality Group: Charles Gunn, Tim Hoffmann, Markus
 * Schmies, Steffen Weissmann.
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * - Neither the name of jReality nor the names of its contributors nor the
 *   names of their associated organizations may be used to endorse or promote
 *   products derived from this software without specific prior written
 *   permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */


package de.jreality.backends.label;

import java.awt.Color;
import java.awt.Font;
import java.awt.Graphics2D;
import java.awt.Rectangle;
import java.awt.RenderingHints;
import java.awt.font.FontRenderContext;
import java.awt.font.TextLayout;
import java.awt.image.BufferedImage;
import java.lang.ref.ReferenceQueue;
import java.lang.ref.WeakReference;
import java.util.HashMap;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.Map;
import java.util.Map.Entry;
import java.util.WeakHashMap;

import javax.swing.SwingConstants;

import de.jreality.geometry.Primitives;
import de.jreality.math.P3;
import de.jreality.math.Pn;
import de.jreality.math.Rn;
import de.jreality.scene.Geometry;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.IndexedLineSet;
import de.jreality.scene.PointSet;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.Transformation;
import de.jreality.scene.data.Attribute;
import de.jreality.scene.data.DataList;
import de.jreality.scene.data.DoubleArrayArray;
import de.jreality.scene.data.IntArray;
import de.jreality.scene.data.IntArrayArray;
import de.jreality.scene.data.StringArray;
import de.jreality.shader.ImageData;

public class LabelUtility {
	
  private LabelUtility() {}

  /************** CACHING *****************/
  
  private static final ReferenceQueue refQueue = new ReferenceQueue();
  private static final WeakHashMap geometryToRefs = new WeakHashMap();
  private static final HashMap refsToImageMaps = new HashMap();
  private static final HashMap refsToAccessTimeMaps = new HashMap();

  private static final Object CACHE_MUTEX = new Object();
  
  // store images for at least ... times not accessed for the geometry
  private static final int MAX_UNUSED_COUNT = 10;
  
  /**
   * maybe we should reuse all keys...
   */
  private static class Key {
    
    final static int TYPE_POINTS=0;
    final static int TYPE_EDGES=2;
    final static int TYPE_FACES=3;
    
    private int type;
    private Font font;
    private Color color;

    private final int hash;
    
    Key(int t, Font f, Color c) {
      if (t<0 || t>3) throw new IllegalArgumentException("no such type");
      type=t; font=f; color=c;
      hash = 37*37*c.hashCode()+37*f.hashCode()+t;
    }
    
    public boolean equals(Object obj) {
      if (obj == null || !(obj instanceof Key)) return false;
      Key k = (Key) obj;
      return k.hashCode() == hashCode() && k.type == type && k.font.equals(font) && k.color.equals(color);
    }
    
    public int hashCode() {
      return hash;
    }
    
    public String toString() {
      return "Key: font="+font+" color="+color;
    }
  }
  
  /**
   * does caching of ImageData objects for geometries and their strings.
   * Images will be deleted if they are not read for more than MAX_CACHE_TIME.
   * 
   * Old Strings will be deleted automatically.
   * 
   * Cache updating is only peformed for the current geometry+type
   * 
   * TODO: it seems that geometries are not dereferenced...
   */
  private static ImageData[] createImages(Geometry geom, int type, StringArray labels, Color color, Font font) {
    ImageData[] ret = new ImageData[labels.getLength()];
    synchronized(CACHE_MUTEX) {
      for (Object ref=refQueue.poll(); ref != null; ref=refQueue.poll()) {
        HashMap del = (HashMap) refsToImageMaps.remove(ref);
        //for (Iterator i = del.keySet().iterator(); i.hasNext(); )
          //LoggingSystem.getLogger(LabelUtility.class).fine("deleted "+i.next());
     }
      WeakReference wref = (WeakReference) geometryToRefs.get(geom);
      if (wref == null) {
        wref=new WeakReference(geom, refQueue);
        geometryToRefs.put(geom, wref);
      }
      // keyToImageMap contains different strToImage maps 
      // for several font/color/type combinations (mapped with Keys)
      HashMap keyToImageMap = (HashMap) refsToImageMaps.get(wref);
      if (keyToImageMap == null) {
        //LoggingSystem.getLogger(LabelUtility.class).fine("create keyToImageMap");
        keyToImageMap = new HashMap();
        refsToImageMaps.put(wref, keyToImageMap);
      }
      
      HashMap keyToAccess = (HashMap) refsToAccessTimeMaps.get(wref);
      if (keyToAccess == null) {
        //LoggingSystem.getLogger(LabelUtility.class).fine("create keyToAccess");
        keyToAccess = new HashMap();
        refsToAccessTimeMaps.put(wref, keyToAccess);
      }
  
      Key key = new Key(type, font, color);
      HashMap strToImages = (HashMap) keyToImageMap.get(key);
      if (strToImages == null) {
        strToImages = new HashMap();
        keyToImageMap.put(key, strToImages);
        keyToAccess.put(key, new int[1]);
        //LoggingSystem.getLogger(LabelUtility.class).fine("created key "+key);
      }      
      int[] accessCount = (int[]) keyToAccess.get(key);
           
      LinkedList remainingStrings=new LinkedList();
      for (int i = 0, n=labels.getLength(); i < n; i++) {
        String str = labels.getValueAt(i);
        ret[i] = (ImageData) strToImages.get(str);
        remainingStrings.add(str);
        if (ret[i] == null) {
          ret[i] = new ImageData(createImageFromString(str, font, color));
          strToImages.put(str, ret[i]);
          //LoggingSystem.getLogger(LabelUtility.class).finer("created imgData for ["+str+"]");
        }
      }
      // store last access
      accessCount[0] = -1;
      
      // delete strToImage for unused keys
      for (Iterator iter = keyToAccess.entrySet().iterator(); iter.hasNext();) {
        Map.Entry e = (Map.Entry) iter.next();
        int[] unusedCnt = (int[]) e.getValue();
        unusedCnt[0]++;
        if (unusedCnt[0] > MAX_UNUSED_COUNT) {
          iter.remove();
          Key removeKey = (Key) e.getKey();
          keyToImageMap.remove(removeKey);
          //LoggingSystem.getLogger(LabelUtility.class).fine("removed images for "+e.getKey());
        }
      }
      
      // delete old strings
      for (Iterator iter = keyToImageMap.entrySet().iterator(); iter.hasNext();) {
        Entry e = (Entry) iter.next();
        Key k = (Key) e.getKey();
        if (key.type != k.type) continue;
        HashMap strToImgs = (HashMap) e.getValue();
        strToImgs.keySet().retainAll(remainingStrings);
      }      
      strToImages.keySet().retainAll(remainingStrings);
    }
    return ret;
  }

  /************** CACHING END *****************/
  
  private static final FontRenderContext frc;
  private static BufferedImage bi;
	//TODO is there a better way to get a FontRenderContext???
	static {
		bi = new BufferedImage(1,1,BufferedImage.TYPE_INT_ARGB);
		frc = bi.createGraphics().getFontRenderContext();
	}

  private static Color TRANSPARENT = new Color(0,0,0,0);
  
  public static ImageData[] createPointImages(PointSet ps, Font f, Color c) {
    DataList dl = ps.getVertexAttributes(Attribute.LABELS);
    if (dl == null) return null;
    StringArray sa = dl.toStringArray();
    return createImages(ps, Key.TYPE_POINTS, sa, c, f);
  }
  
  public static ImageData[] createEdgeImages(IndexedLineSet ls, Font f, Color c) {
    DataList dl = ls.getEdgeAttributes(Attribute.LABELS);
    if (dl == null) return null;
    StringArray sa = dl.toStringArray();
    return createImages(ls, Key.TYPE_EDGES, sa, c, f);
  }

  public static ImageData[] createFaceImages(IndexedFaceSet fs, Font f, Color c) {
    DataList dl = fs.getFaceAttributes(Attribute.LABELS);
    if (dl == null) return null;
    StringArray sa = dl.toStringArray();
    return createImages(fs, Key.TYPE_FACES, sa, c, f);
  }

  public static BufferedImage createImageFromString(String s, Font f, Color color) {
	  return createImageFromString(s, f, color, TRANSPARENT);
  }
  
  public static BufferedImage createImageFromString(String s, Font f, Color foreground, Color background) {
	  String[] lines = s.split("\n");
	  return createImageFromStrings(lines,f,foreground, background);
//	  //Rectangle r = f.getStringBounds(s,frc).getBounds();
//	  if (s==null || s.length()==0) {
//		  if (background.equals(TRANSPARENT)) return bi;
//		  else s = " ";
//	  }
//	  if (f == null) f = new Font("Sans Serif",Font.PLAIN,48);
//	  TextLayout tl = new TextLayout(s,f,frc);
//	  Rectangle r = tl.getBounds().getBounds();
//	  
//	  // HACK: the previous implementation failed for strings without descent...
//	  // I got cut-off in the vertical dir, so i added a border of width 2
//	  int height = (int) f.getLineMetrics(s,frc).getHeight();//new TextLayout("fg", f, frc).getBounds().getBounds().height;
//    int width = (r.width+20);
//    
//    BufferedImage img = new BufferedImage(width,height,BufferedImage.TYPE_INT_ARGB);
//	  Graphics2D g = (Graphics2D) img.getGraphics();
//	  g.setBackground(background);
//	  g.clearRect(0,0,width,height);
//	  g.setColor(foreground);
//	  g.setFont(f);
////	  LineMetrics lineMetrics = f.getLineMetrics(s,frc).getHeight();
//	  g.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_ON);
//		
//	  final float border = height - tl.getDescent();
//
//    g.drawString(s,0,border);
//	  return img;
  }
 
  public static BufferedImage createImageFromStrings(String[] ss, Font f, Color foreground, Color background) {
	  // HACK: the previous implementation failed for strings without descent...
	  // I got cut-off in the vertical dir, so i added a border of width 2
	  int width = 0, height = 0, hh[] = new int[ss.length];
	  float border = 0.0f;
	  if (f == null) f = new Font("Sans Serif",Font.PLAIN,48);
	  // process the strings to find out how large the image needs to be
	  // I'm not sure if I'm handling the border correctly: should a new border
	  // be added for each string?  Or only for the first or last?
		for (int i = 0; i < ss.length; ++i) {
			String s = ss[i];
			if (s == null || s.length() == 0) {
				if (background.equals(TRANSPARENT))
					return bi;
				else
					s = " ";
			}
			TextLayout tl = new TextLayout(s, f, frc);
			Rectangle r = tl.getBounds().getBounds();
			hh[i] = (int) f.getLineMetrics(s, frc).getHeight();
			height += hh[i];
			int tmp = (r.width + 20);
			if (tmp > width) width = tmp;
			float ftmp = hh[i] - tl.getDescent();
			if (ftmp > border) border = ftmp;
		}
		BufferedImage img = new BufferedImage(width, height,BufferedImage.TYPE_INT_ARGB);
		Graphics2D g = (Graphics2D) img.getGraphics();
		g.setBackground(background);
		g.clearRect(0, 0, width, height);
		g.setColor(foreground);
		g.setFont(f);
		// LineMetrics lineMetrics = f.getLineMetrics(s,frc).getHeight();
		g.setRenderingHint(RenderingHints.KEY_ANTIALIASING,
				RenderingHints.VALUE_ANTIALIAS_ON);
		height = 0;
		for (int i = 0; i < ss.length; ++i) {
			g.drawString(ss[i], 0, height + border);
			height += hh[i];
		}
		return img;
  	}
  	private static final IndexedFaceSet bb = Primitives.texturedQuadrilateral(new double[]{0,1,0,1,1,0,1,0,0,0,0,0});

  	public static SceneGraphComponent sceneGraphForLabel(SceneGraphComponent sgc, double xscale, double yscale,double[] offset, int alignment, double[] camToObj, double[] position)  {
  		if (sgc == null) sgc = new SceneGraphComponent();
  		if (sgc.getGeometry() == null) {
  			sgc.setGeometry(bb);
  		}
  		if (sgc.getTransformation() == null)	sgc.setTransformation(new Transformation());
   		sgc.getTransformation().setMatrix(LabelUtility.calculateBillboardMatrix(null, xscale, yscale, offset, alignment, camToObj, position, Pn.EUCLIDEAN ));

  		return sgc;
  	}

  public static double[] positionFor(int ind, DoubleArrayArray a, IntArrayArray indexed) {
    if (indexed == null) return a.getValueAt(ind).toDoubleArray(null);
    double[] ret = null;
    IntArray part = indexed.getValueAt(ind);
    double[] tmp=null;
    for (int i = 0; i < part.getLength(); i++) {
      tmp = a.getValueAt(part.getValueAt(i)).toDoubleArray(tmp);
      if (ret == null) ret = (double[]) tmp.clone();
      else {
        for (int j = 0; j < tmp.length; j++) ret[j] += tmp[j];
      }
    }
    for (int j = 0; j < tmp.length; j++) ret[j] /= part.getLength();
    return ret;
  }

public static double[] calculateBillboardMatrix(double[] result, 
		double xscale, 
		double yscale, 				// scaling factors for the billboard
		double[] xyzOffset,			// an offset in "billboard" coordinate system
		int alignment,	     		// alignment of billboard in compass-direction from anchor point (using SwingConstants)
		double[] cameraToObject, 	// the transformation from camera to object coordinates
		double[] point, 			// the position of the anchor point in object coordinate system
		int metric)	{
	if (result == null) result = new double[16];
	// TODO fix this method for noneuclidean metrics
	// TODO the following call perhaps should return a determinant-1 matrix (throw out scaling)
    double[] orientation = P3.extractOrientationMatrix(null, cameraToObject, P3.originP3, metric);
    double[] scale = P3.makeStretchMatrix(null, xscale, yscale, 1.0);
    //calculate translation for alignment
    double align=0, valign=0;  // default
    switch (alignment) {
    	case SwingConstants.NORTH  : align=-xscale/2; break;
    	case SwingConstants.EAST   : valign=-yscale/2; break;
    	case SwingConstants.SOUTH  : align=-xscale/2; valign=-yscale; break;
    	case SwingConstants.WEST   : align=-xscale;   valign=-yscale/2; break;
    	case SwingConstants.CENTER : align=-xscale/2; valign=-yscale/2; break;
    	//case SwingConstants.NORTH_EAST : default
    	case SwingConstants.SOUTH_EAST : valign=-yscale; break;
    	case SwingConstants.SOUTH_WEST : align=-xscale; valign=-yscale; break;
    	case SwingConstants.NORTH_WEST : align=-xscale; break;
    }
//    System.err.println("Calculating translation for metric "+metric);
    metric = Pn.EUCLIDEAN;
    double[] euclideanTranslation = P3.makeTranslationMatrix(null, Rn.add(null, xyzOffset, new double[]{align, valign, 0, 0}), metric); //Pn.EUCLIDEAN);
    double[] pointTranslation;
    if (Double.isNaN(point[0])) pointTranslation = Rn.identityMatrix(4);
    else pointTranslation = P3.makeTranslationMatrix(null, point, metric);

    Rn.times(result, pointTranslation, Rn.times(null, orientation, Rn.times(null, euclideanTranslation, scale)));
	return result;
}

}
