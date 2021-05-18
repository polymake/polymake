/* Copyright (c) 1997-2021
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

package de.tuberlin.polymake.common;

/**
 * A class for exceptions in the SharedMemoryMatrix
 *
 * @author Ralf Hoffmann
 */
public class SharedMemoryMatrixException extends java.lang.Exception {

	private static final long serialVersionUID = 1L;

	/**
	* Constructs a new SharedMemoryMatrixException with null as its detail message.
	*/
	public SharedMemoryMatrixException() {
		super();
	}
	
	/**
	* Constructs a new SharedMemoryMatrixException with the specified detail message.
	*
	* @param msg	the detail message
	*/
	public SharedMemoryMatrixException( String msg ) {
		super( msg );
	}
}
