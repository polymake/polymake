/* Copyright (c) 1997-2022
   Ewgenij Gawrilow, Michael Joswig, and the polymake team
   Technische Universität Berlin, Germany
   https://polymake.org

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2, or (at your option) any
   later version: http://www.gnu.org/licenses/gpl.txt.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
--------------------------------------------------------------------------------
 */

package de.tuberlin.polymake.common.io;

import java.io.IOException;
import java.util.Properties;

import de.tuberlin.polymake.common.SharedMemoryMatrixException;
import de.tuberlin.polymake.common.geometry.PointSet;

/**
 * 
 * @author Thilo Rörig
 */
public abstract class PolymakeParser {

	/** HashSet containing non-jvx parameters */
	protected Properties parameters = null;

	/** HashSet containing interactive parameters */
	protected Properties iparameters = null;

	/** String containing last error message */
	protected String error = null;

	/** String containing last warning */
	protected String warning = null;

	/** Name of parsed geometry */
	protected String name = "NoName";

	// protected String mode = "stop";

	/**
	 * Create a new parser for Javaview files created by polymake containing
	 * additional parameters specified by params. These params are child
	 * elements of the geometry node in the Javaview jvx file format.
	 * 
	 * @param params
	 *            additional non Javaview parameters.
	 */
	public PolymakeParser(String[] params) {
		parameters = new Properties();
		iparameters = new Properties();
		for (int i = 0; i < params.length; ++i) {
			parameters.setProperty(params[i], "null");
		}
	}

	/**
	 * get the value of a parameter.
	 * 
	 * @param param
	 *            name of parameter
	 * @return value of parameter or null if there is any
	 */
	public String getParam(String param) {
		return parameters.getProperty(param);
	}

	/**
	 * determine whether a parameter is interactive or not: null means it is not
	 * interactive, 1 means it is a direct response parameter and 0 means it is
	 * interactive, but not direct.
	 * 
	 * @param param
	 *            name of parameter
	 * @return value of parameter or null if there is any
	 */
	public String getInteractiveParam(String param) {
		return iparameters.getProperty(param);
	}

	/** get all parameters */
	public Properties getParameters() {
		return parameters;
	}

	/** get all parameters */
	public Properties getInteractiveParameters() {
		return iparameters;
	}

	/** get the last error message */
	public String getError() {
		return error;
	}

	/** get the last warning */
	public String getWarning() {
		return warning;
	}

	public boolean stop() {
		return (getParam("continue") != null && getParam("continue")
				.equals("0")) ? true : false;
	}

	/**
	 * @param in
	 *            BufferedReader for textual input which will be parsed
	 * @throws IOException
	 *             if problems reading from in occur
	 * @throws SharedMemoryMatrixException
	 */
	public abstract void parse(java.io.BufferedReader in, PointSet ps)
			throws IOException, SharedMemoryMatrixException;
}
