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


package de.jreality.util;

import java.io.BufferedInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.Reader;
import java.io.UnsupportedEncodingException;
import java.net.MalformedURLException;
import java.net.URI;
import java.net.URISyntaxException;
import java.net.URL;

/**
 * Abstraction of an input source, whether given as a file on the local file system, 
 * a URL, or as a Java resource.
 * Technically an instance covers an {@link java.io.InputStream}.
 * 
 * @author Steffen Weissman
 * TODO: document this.
 */
public final class Input
{
  private InputStream inputStream;
  private String description;
  private File sourceFile;
  private URL sourceURL;

  public Input(URL url) throws IOException
  {
    this(url.toString(), url.openStream());
    this.sourceURL=url;
  }
  public Input(File file) throws FileNotFoundException
  {
    this(file.toString(), new FileInputStream(file));
    this.sourceFile=file;
  }
  public Input(String description, InputStream is)
  {
    this.description=description;
    this.inputStream=new BufferedInputStream(is);
  }
  
  /**
   * @return the InputStream for this Input
   */
  public InputStream getInputStream()
  {
    return inputStream;
  }
  /**
   * Creates a Reader for this Input
   * 
   * @return a reader for the covered InputStream
   */
  public Reader getReader()
  {
    return new InputStreamReader(inputStream);
  }
  
  /**
   * Creates a Reader for this Input with given encoding.
   *  
   * @param encoding the encoding to use for the created Reader
   * @return a reader for the covered InputStream with the given encoding
   * @throws UnsupportedEncodingException
   */
  public Reader getReader(String encoding) throws UnsupportedEncodingException
  {
    return new InputStreamReader(inputStream, encoding);
  }
  
  /**
   * Tries to resolve a Resource relative to this Input. This works
   * only for Files and URLs.
   * 
   * @param name the relative name for the resource.
   * @return an Input for the relative resource.
   * @throws IOException if no such relative resource was found.
   */
  public Input getRelativeInput(String name) throws IOException
  {
    if(sourceFile!=null)
    {
      File p=sourceFile.getParentFile();
      return new Input((p==null)? new File(name): new File(p, name));
    }
    else if(sourceURL!=null)
    {
      return new Input(new URL(sourceURL, name));
    }
    else throw new IOException("cannot resolve \""
      +name+"\" relative to \""+description+'"');
  }
  
  /**
   * Tries to create an Input for the given name as follows:
   * <li> if there is a System resource with the given name, this is returned.
   * <li> if the given name is absolute, the corresponding Input is returned.
   * <li> if the input can be resolved relative to an underlying file or URL,
   * this Input is returned.
   * <li> Last try is to find the resource relative to the current directory.
   * 
   * @param name the name of the resource.
   * @return an Input for the given name.
   * @throws IOException if sth. goes wrong.
   */
  public Input resolveInput(String name) throws IOException {
    try {
      // TODO: is this the right way to find resources from classpath?
      URL test = ClassLoader.getSystemResource(name);
      if (test != null) {
        return new Input(test);
      }
      URI uri = new URI(name);
      if (uri.isAbsolute()) return new Input(uri.toURL());
      URI base;
      if (sourceFile != null)
        base = sourceFile.toURI();
      else if (sourceURL != null)
        base = new URI(sourceURL.toExternalForm());
      else
        //     throw new IOException("cannot resolve \""
        //        +name+"\" relative to \""+description+'"');
        base = new URI("file:"
            + Secure.getProperty("user.dir", "/").replace(File.separatorChar,
                '/')).normalize();
      URI resolved = base.resolve(uri);
      if (sourceFile != null && resolved.getScheme().equals("file"))
        return new Input(new File(resolved));
      else
        return new Input(resolved.toURL());
    } catch (URISyntaxException e) {
      throw new MalformedURLException(e.getMessage());
    } catch (IllegalArgumentException e2) {
      throw new MalformedURLException(e2.getMessage());
    }
  }
    
  /**
   * returns a {@link java.io.File} for this Input if possible.
   * @return the File of this Input.
   * 
   * @throws UnsupportedOperationException if this Input is not from a file.
   */
  public File toFile() throws UnsupportedOperationException {
    if (sourceFile == null) throw new UnsupportedOperationException("not a file");
    return sourceFile;
  }
  
  /**
   * returns a {@link java.net.URL} for this Input if possible.
   * @return the URL of this Input.
   * 
   * @throws MalformedURLException if this Input can't be converted to a URL.
   */
  public URL toURL() throws MalformedURLException {
    if (sourceURL != null) return sourceURL;
    if (sourceFile != null) return sourceFile.toURL();
    throw new MalformedURLException("cannot convert "+description+" to URL.");
  }
  public String getDescription() {
      return description;
  }
  
  public String toString() {
      return "Input: "+description;
  }
/**
   * factory method for creating an {@link Input}.
   * @param url the URL to create the Input for.
   * @return an Input that covers the given URL.
   * @throws IOException
   */
  public static Input getInput(URL url) throws IOException
  {
    return new Input(url);
  }
/**
   * factory method for creating an {@link Input}.
   * @param file the file to create the Input for.
   * @return an Input that covers the given File.
   * @throws IOException
   */
  public static Input getInput(File file) throws IOException
  {
    return new Input(file);
  }
/**
   * factory method for creating an {@link Input}.
   * @param in the InputStream to create the Input for.
   * @param description a String describing the type of input.
   * @return an Input that covers the given InputStream.
   * @throws IOException
   */
  public static Input getInput(String description, InputStream in) throws IOException
  {
    return new Input(description, in);
  }
/**
   * searches for the given resource name as follows:
   * <li> if resourceName contains :// we try to load it as a URL
   * <li> if resourceName is an absolute filename the corresponding Input is created
   * <li> resourceName is searched in the classpath
   * <li> resourceName is searched relative to System.getProperty(SystemProperties.JREALITY_DATA)
   * <li> resourceName is searched relative to the current dir
   * 
   * @param resourceName the name of the resource to look for
   * @return an Input for the given resource name.
   * @throws IOException
   */
  public static Input getInput(String resourceName) throws IOException {
    if (resourceName.indexOf("://") != -1) {
      try {
        return getInput(new URL(resourceName));
      } catch (IOException e) {
        // not found maybe it is a file...
      }
    }
    File f = new File(resourceName);
    if (f.isAbsolute()) return Input.getInput(f);
    URL test = Thread.currentThread().getContextClassLoader().getResource(resourceName);
    Input ret = (test == null ? null : new Input(test));
    if (ret != null) return ret;
    String currentDir = null;
    try {
      currentDir = Secure.getProperty(SystemProperties.JREALITY_DATA, SystemProperties.JREALITY_DATA_DEFAULT);
    } catch (SecurityException se) {
      // webstart
    }
    if (currentDir != null) {
        try {
	      File dir = new File(currentDir);
	      File f1 = new File(dir, resourceName);
	      if (f1.exists()) return new Input(f1);
        } catch (SecurityException se) {
            // webstart
        }
    }
    try {
      currentDir = Secure.getProperty("user.dir");
    } catch (SecurityException se) {
      // webstart
    }
    if (currentDir != null) {
        try {
	      File dir = new File(currentDir);
	      File f1 = new File(dir, resourceName);
      if (f1.exists()) return new Input(f1);
        } catch (SecurityException se) {
            // webstart
        }
    }
    throw new IOException("Resource not found ["+resourceName+"]");
  }
  
  /**
   * Get a fresh version of this Input - i.e. used to re-read files.
   * Workes only for files and URLs.
   * 
   * @return an Input for the same content, but not-yet used InputStream.
   * @throws IOException
   */
  public Input copy() throws IOException {
		if (sourceFile != null) return new Input(sourceFile);
		if (sourceURL != null) return new Input(sourceURL);
		throw new UnsupportedOperationException("only for files and URLs");
	}

  public String getContentAsString() throws IOException {
  	StringBuilder sb = new StringBuilder();
  	Reader r = getReader();
  	char[] buf = new char[1024];
  	int read;
  	while ((read=r.read(buf))!=-1) sb.append(buf, 0, read);
  	return sb.toString();
  }
  
}
