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


package de.jreality.ui.viewerapp;

import java.io.File;
import java.util.HashSet;
import java.util.LinkedHashSet;
import java.util.Set;

import javax.imageio.ImageIO;

import de.jreality.reader.Readers;


/**
 * @author msommer
 */
public class FileFilter extends javax.swing.filechooser.FileFilter {
  
  private HashSet<String> extensions;
  private String preferred;
  private String description;
  private boolean showExtensionList = true;
  

  public FileFilter() {
    this(null);
  }

  
  public FileFilter(String description, String... extensions) {
    setDescription(description);
    this.extensions = new LinkedHashSet<String>();
    for (int i = 0; i < extensions.length; i++)
      addExtension(extensions[i]);
  }
  

  @Override
	public String toString() {
		return getDescription();
	}
  
  @Override
  public boolean accept(File f) {
    if (f == null) return false;
    if(f.isDirectory()) return true;
      
    String extension = getFileExtension(f);
    return (extension != null && 
        (extensions.contains(extension) ||
            extensions.contains(extension.toLowerCase()) ||
            extensions.contains(extension.toUpperCase())) );
  }


  @Override
  public String getDescription() {
    
    if (!showExtensionList || extensions.isEmpty()) 
      return description;
    
    String extensionList = null;
    for (String extension : extensions) {
      if (extensionList == null) extensionList = " (*."+extension;
      else extensionList += ", *."+extension;
    }
    extensionList += ")";
    
    return description + extensionList;
  }
  
  
  public void setDescription(String description) {
    this.description = description;
  }
  
  
  public void addExtension(String extension) {
    extensions.add(extension);
  }

  
  public void setPreferredExtension(String preferred) {
    addExtension(preferred);
    this.preferred = preferred;
  }
  
  
  public String getPreferredExtension() {
    if (preferred == null && !extensions.isEmpty())
      return extensions.iterator().next();  //return first in set

    return preferred;
  }
  
  
  public static String getFileExtension(File f) {
    if (f != null) {
      String filename = f.getName();
      int i = filename.lastIndexOf('.');
      if(i>0 && i<filename.length()-1)
        return filename.substring(i+1).toLowerCase();
    }
    return null;
  }
  

  public boolean isShowExtensionList() {
    return showExtensionList;
  }


  public void setShowExtensionList(boolean showExtensionList) {
    this.showExtensionList = showExtensionList;
  }
  
  
  public static FileFilter createJRealityDataFilter() {
	  FileFilter f = new FileFilter("jReality 3D data files") {
		  @Override
		  public boolean accept(File f) {
			  if (f.isDirectory()) return true;
			  String filename = f.getName().toLowerCase();
			  return (Readers.findFormat(filename) != null);
		  }
	  };
	  f.setShowExtensionList(false);
	  
	  return f;
  }
  
  
  public static javax.swing.filechooser.FileFilter[] createImageWriterFilters() {
  	
  	//get existing writer formats
		String writerFormats[] = ImageIO.getWriterFormatNames();
		//usually [bmp, jpg, jpeg, png, wbmp]
		String[] known = new String[]{"bmp","jpg","jpeg","png", "wbmp","tiff","tif"};
		//get remaining formats ignoring case
		Set<String> special = new HashSet<String>();
		outer: for (int i = 0; i < writerFormats.length; i++) {
			final String ext = writerFormats[i].toLowerCase();
			for (int j = 0; j < known.length; j++) {
				if (known[j].equals(ext)) continue outer;
			}
			special.add(ext);
		}

		Set<FileFilter> filters = new LinkedHashSet<FileFilter>();
		//add general filter
		FileFilter general = new FileFilter("All Image Files", known);
		for (String s : special) general.addExtension(s);
		general.setPreferredExtension("png");
		filters.add(general);
		//add known filter
		filters.add(new FileFilter("PNG Image", "png"));
		filters.add(new FileFilter("JPEG Image", "jpg", "jpeg"));
		//add tiff filter if writer exists
		try { Class.forName("javax.media.jai.JAI");
		filters.add(new FileFilter("TIFF Image", "tiff", "tif"));
		} catch (ClassNotFoundException e) {}
		filters.add(new FileFilter("BMP Image", "bmp"));
		filters.add(new FileFilter("Wireless BMP Image", "wbmp"));
		//add filters for special writer formats
		for (String s : special)
			filters.add(new FileFilter(s.toUpperCase()+" Image", s));

		//convert to array
		FileFilter[] ff = new FileFilter[filters.size()];
		return filters.toArray(ff);
  }
  
  
  public static javax.swing.filechooser.FileFilter[] createImageReaderFilters() {
  	
  	//get existing reader formats
		String readerFormats[] = ImageIO.getReaderFormatNames();
		//usually [bmp, gif, jpg, jpeg, png, wbmp]
		String[] known = new String[]{"bmp", "gif", "jpg","jpeg","png", "wbmp"};
		//get remaining formats ignoring case
		Set<String> special = new HashSet<String>();
		outer: for (int i = 0; i < readerFormats.length; i++) {
			final String ext = readerFormats[i].toLowerCase();
			for (int j = 0; j < known.length; j++) {
				if (known[j].equals(ext)) continue outer;
			}
			special.add(ext);
		}

		Set<FileFilter> filters = new LinkedHashSet<FileFilter>();
		//add general filter
		FileFilter general = new FileFilter("All Image Files", known);
		for (String s : special) general.addExtension(s);
		general.setPreferredExtension("png");
		filters.add(general);
		//add known filter
		filters.add(new FileFilter("PNG Image", "png"));
		filters.add(new FileFilter("JPEG Image", "jpg", "jpeg"));
		filters.add(new FileFilter("GIF Image", "gif"));
//		//add tiff filter if writer exists
//		try { Class.forName("javax.media.jai.JAI");
//		filters.add(new FileFilter("TIFF Image", "tiff", "tif"));
//		} catch (ClassNotFoundException e) {}
		filters.add(new FileFilter("BMP Image", "bmp"));
		filters.add(new FileFilter("Wireless BMP Image", "wbmp"));
		//add filters for special writer formats
		for (String s : special)
			filters.add(new FileFilter(s.toUpperCase()+" Image", s));

		//convert to array
		FileFilter[] ff = new FileFilter[filters.size()];
		return filters.toArray(ff);
  }
  
}