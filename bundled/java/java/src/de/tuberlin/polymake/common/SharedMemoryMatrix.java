/* Copyright (c) 1997-2023
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

import de.tuberlin.polymake.common.geometry.PointSet;
import de.tuberlin.polymake.common.geometry.PolymakePoint;

/**
 * A class that establishes the connection of the polymake geometry
 * classes to a shared memory segment containing a matrix of doubles.
 * 
 * @author Ralf Hoffmann, Thilo Rörig
 */

public class SharedMemoryMatrix {

	/**
	 * The mapped address
	 */
	private long addr = -1l;

	public long getAddr() { return addr; };

	double[] matrix;
	
	/**
	 * Create a new SharedMemoryMatrix attached to a shared memory
	 * segment containing a C++ SharedMemoryMatrix<double> object.
	 * 
	 * @param key	the key of the shared memory segment
	 */
	public SharedMemoryMatrix(int key) throws SharedMemoryMatrixException {
		attachToShm(key);
	}

	public native void finalize() throws SharedMemoryMatrixException;

	/**
	 * wrapper of the shmat system call
	 */
	private native void attachToShm(int key) throws SharedMemoryMatrixException;

	/**
	 * Copy the coordinates contained in the shared memory
	 * segment into the pointset <code>ps</code>
	 * 
	 * @param ps	the PointSet to which the coordinates are copied
	 */
	public native void copyCoords(PointSet ps) throws SharedMemoryMatrixException;
	
	
	/**
	 * Copy one row of the matrix contained
	 * in the shared memory segment into the given PolymakePoint
	 * 
	 * @param index	row index
	 * @param pt	the PolymakePoint to which the coords are copied
	 */
	public native void copyCoords(int index, PolymakePoint pt) throws SharedMemoryMatrixException;
	
	
	/**
	 * Set the coordinates of the matrix in the shared memory segment
	 * to the coordinates of the PointSet
	 * 
	 * @param ps	PointSet containing the new coordinates
	 */
	public native void setCoords(PointSet ps) throws SharedMemoryMatrixException;
	
	/**
	 * Set one row of the matrix in the shared memory segment
	 * to the coordinates of the PolymakePoint <code>pt</code>
	 * 
	 * @param index	row index
	 * @param pt	PolymakePoint containing the new coordinates
	 */
	public native void setCoords(int index, PolymakePoint pt) throws SharedMemoryMatrixException;

	public native int getNPoints();

	public native int getDim();
}
