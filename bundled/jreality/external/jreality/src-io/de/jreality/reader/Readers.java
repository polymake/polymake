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


package de.jreality.reader;

import java.io.File;
import java.io.IOException;
import java.net.URL;
import java.util.HashMap;

import de.jreality.scene.Appearance;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.util.Input;

/**
 * Entry point to load a jReality SceneGraph from some sort of resource.
 */
public final class Readers {

	static HashMap<String, Class<? extends SceneReader>> formatToReader = new HashMap<String, Class<? extends SceneReader>>(); 
	static HashMap<String, String> endingToFormat = new HashMap<String, String>();
	
	static {
		registerFileEndings("FE", "fe");
		registerFileEndings("MESH", "mesh");
		registerFileEndings("POLY", "top");
		registerFileEndings("POV", "pov");
		registerFileEndings("BSH", "bsh");
		registerFileEndings("LAS", "las");
		//registerFileEndings("OBJTexture", "obj");
		registerFileEndings("OBJ", "obj");
		registerFileEndings("3DS", "3ds");
		registerFileEndings("JVX", "jvx");
		registerFileEndings("JRS", "jrs");
		registerFileEndings("PTS", "pts");
		registerFileEndings("STL", "stl");
		registerFileEndings("MATHEMATICA", "m");
		registerFileEndings("MSMS", "fe");
		registerFileEndings("OOGL", "off", "quad", "mesh", "vect", "bez", "grp", "oogl", "list", "inst");
		registerFileEndings("VRML", "wrl", "vrml", ".iv");
	}
	
	// avoid instanciating this class
	private Readers() {}
	
	/**
	 * Register a file ending for a format. Overwrites previous and default registrations.
	 * 
	 * @param fileEnding the file ending.
	 * @param format the format.
	 */
	public static void registerFileEndings(String format, String... fileEndings) {
		for (String fe : fileEndings)
			endingToFormat.put(fe.toLowerCase(), format);
	}
	
	/**
	 * Register a reader class to read a certain format.
	 * 
	 * @param format the format
	 * @param reader the reader class
	 */
	public static void registerReader(String format, Class<? extends SceneReader> reader) {
		formatToReader.put(format, reader);
	}
	
	/**
	 * returns a reader for the given format if available - null if not.
	 * @param format String representation for the format to read.
	 * @return a matching reader or null.
	 */
	@SuppressWarnings("unchecked")
	public static SceneReader findReader(String format)
	{
		Class<? extends SceneReader> clazz = formatToReader.get(format);
		if (clazz == null) {
			try
			{
				clazz = (Class<? extends SceneReader>) Class.forName("de.jreality.reader.Reader"+format);
			} catch (ClassNotFoundException e)
			{
				return null;
			}
		}
		Exception ex;
		try {
			return clazz.newInstance();
		} catch (InstantiationException e) {
			ex = e;
		} catch (IllegalAccessException e) {
			ex=e;
		}
		IllegalStateException rtex=new IllegalStateException("invalid reader");
		rtex.initCause(ex);
		throw rtex;
	}

	/**
	 * Reads the scene from the given input with the given SceneReader.
	 * @param reader the SceneReader to read with.
	 * @param input the Input to read with the given SceneReader.
	 * @return the root component of the read scene.
	 * @throws IOException
	 */
	public static SceneGraphComponent read(SceneReader reader, Input input)
	throws IOException
	{
		reader.setInput(input);
		return reader.getComponent();
	}

	/**
	 * Reads the scene from the given input with the SceneReader for the given format.
	 * @param format the format of the given input.
	 * @param input the Input to read with the SceneReader for the given format.
	 * @return the root component of the read scene.
	 * @throws IOException
	 */
	public static SceneGraphComponent read(String format, Input input)
	throws IOException
	{
		return read(findReader(format), input);
	}

	/**
	 * reads the given file with the matching reader for the file ending.
	 * @param file the file to read.
	 * 
	 * @throws IOException
	 */
	public static SceneGraphComponent read(File file) throws IOException {
		return read(Input.getInput(file));
	}

	/**
	 * reads the given url with the matching reader for the file ending.
	 * NOTE: the URL must end with a supported file extension.
	 * 
	 * @param url the url to read.
	 * 
	 * @throws IOException
	 */
	public static SceneGraphComponent read(URL url) throws IOException {
		return read(Input.getInput(url));
	}

	/**
	 * determines the format of the given input depending on the file ending
	 * of the URL. That means it can only read Inputs based on filesand URLs.
	 * 
	 * @param input
	 * @return rhe root component of the read input
	 * 
	 * @throws IOException
	 */
	public static SceneGraphComponent read(Input input) throws IOException {
		String lc = input.getDescription();
		String format = findFormat(lc);
		if (format == null) throw new IllegalArgumentException("unknown file format");
		SceneGraphComponent sgc = read(format, input);
		if (sgc.getAppearance() == null) sgc.setAppearance(new Appearance());
		return sgc;
	}

	public static String findFormat(String resourceName) {
		String ending = getEnding(resourceName);
		return endingToFormat.get(ending); //NULL if unknown format (needed for <jReality 3D data files> FileFilter)
	}

	private static String getEnding(String resource) {
		String resourceName = resource.toLowerCase();
		int idx = resourceName.lastIndexOf('.');
		if (idx == -1) return "";
		return resourceName.substring(idx+1);
	}

}
