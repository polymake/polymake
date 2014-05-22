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

/** This class provides utilities for arrays as static methods.
    <p />
    Most operation performed on the <i>target</i> array can imply
    a change of its length. Those methods create a new array and
    return this instead of the original which is only returned
    if it has already the right length.
    @author	Markus Schmies
 */

public final class ArrayUtility {

    /**Default threshold for float epsilon tests is 1e-6.*/
    final public static float   FLOAT_EPS = 1e-6f;
    /**Default threshold for double epsilon tests is 1e-12.*/
    final public static double DOUBLE_EPS = 1e-12;

    private ArrayUtility() {}

    /** Test whether arrays <i>a</i> and <i>b</i> are equal using an epsilon test.
	@param a double array
	@param b double array
	@param eps threshold for epsilon test */
    public static final boolean equal( final double [] a, final double [] b, final double eps ) {
	if( a == b )
	    return true;

	if( a == null || b == null )
	    return false;

	if( a.length != b.length )
	    return false;

	final int length = a.length;

	for( int i=0; i<length; i++ )
	    if( Math.abs( a[i] - b[i] ) > eps )
		return false;

	return true;
    }

    /** Test whether arrays <i>a</i> and <i>b</i> are equal using an epsilon test.
	{@link ArrayUtility#DOUBLE_EPS}
	@param a double array
	@param b double array */
    public static final boolean equal( final double [] a, final double [] b ) {
	return equal( a, b, DOUBLE_EPS );
    }

    /** Returns copy of <i>source</i> which is the <i>target</i> if it is not
	<i>null</i> and has the right size. */
    public static final double [] copy( final double [] source, final double [] target ) {
	if( target == null || target.length != source.length )
	    return (double[])source.clone();

	System.arraycopy( source, 0, target, 0, source.length );

	return target;
    }

    /** Just returns <i>target</i> if its length equal <i>size</i>.
	Otherwise, this includes <i>target</i> is null, it creates
	an array of length <i>size</i> and returns this instead. */
    public static final double [] size( final double [] target, final int size ) {
	return target != null && target.length == size ? target : new double[size];
    }

    /** Like {@link ArrayUtility#size(double[],int)} but copies also the contents of
	the <i>target</i> if nessecary. */
    public static final double [] resize( final double [] target, final int size ) {
	if( target != null && target.length == size )
	    return target;

	double [] newTarget = new double [size];

	System.arraycopy( target, 0, newTarget, 0, Math.min( size, target.length ) );

	return newTarget;
    }

    /** Returns an array containing <i>numOfTimes</i> copies of the
	contens of <i>values</i>.
	The method uses <i>target</i> if it is not <code>null</code> and has the right size. */
    public static final double [] unrole( double [] target,
					  final double [] values, final int numOfTimes ) {
	int length = values.length * numOfTimes;

	if( target == null || target.length != values.length * numOfTimes )
	    target = new double[ length ];

	System.arraycopy( values, 0, target, 0, values.length );

	int i=values.length;
	for( ; 2*i < length; i *= 2 )
	    System.arraycopy( target, 0, target, i, i );

	System.arraycopy( target, 0, target, i, length - i );

	return target;
    }

    /** Retuns an array containing the contents of <i>con</i> and
	<i>cat</i>.
	The method uses <i>target</i> if it is not <code>null</code> and has the right size. */
    public static final double [] concat( double [] target,
					  final double [] con, final double [] cat ) {
	if( target == null || target.length != con.length + cat.length )
	    target = new double[ con.length + cat.length ];

	System.arraycopy( con, 0, target, 0,          con.length );
	System.arraycopy( cat, 0, target, con.length, cat.length );

	return target;
    }

    /** Fills <i>target</i> with <i>value</i>. */
    public static final void fill( final double [] target, final double value ) {
	fill( target, 0, target.length, value );
    }

    /** Fills <i>target</i> from <i>postion</i> to <i>postion+length-1</i>
	with <i>value</i>. */
    public static final void fill( final double [] target, final int position,
				   final int length, final double value ) {
	if( length < 1 ) return;

	target[position + 0]=value;
	if( length < 2 ) return;
	target[position + 1]=value;
	if( length < 3 ) return;
	target[position + 2]=value;
	if( length < 4 ) return;
	target[position + 3]=value;
	if( length < 5 ) return;
	target[position + 4]=value;

	int i=5;
	for( ; 2*i < length; i *= 2 )
	    System.arraycopy( target, position, target, position + i, i );

	System.arraycopy( target, position, target, position + i, length - i );
    }


    /** Returns the sum of <i>target</i>. */
    public static final double sum( final double [] target ) {
	return sum( target, 0, target.length );
    }

    /** Returns the sum of <i>target</i> from <i>postion</i> to <i>postion+length-1</i>. */
    public static final double sum( final double [] target,
				       final int position, final int length ) {
	double sum = 0;
	for( int i=position; i<position+length; i++ )
	    sum += target[i];
	return sum;
    }

    /** Returns the minimum of <i>target</i>. */
    public static final double min( final double [] target ) {
	return min( target, 0, target.length );
    }

    /** Returns the minimum of <i>target</i> from <i>postion</i> to <i>postion+length-1</i>. */
    public static final double min( final double [] target,
				       final int position, final int length ) {
	double min = target[position];
	for( int i=position; i<position+length; i++ )
	    if( min > target[i] )
		min = target[i];
	return min;
    }


    /** Returns the maximum of <i>target</i>. */
    public static final double max( final double [] target ) {
	return max( target, 0, target.length );
    }

    /** Returns the maximum of <i>target</i> from <i>postion</i> to <i>postion+length-1</i>. */
    public static final double max( final double [] target,
				       final int position, final int length ) {
	double max = target[position];
	for( int i=position; i<position+length; i++ )
	    if( max < target[i] )
		max = target[i];
	return max;
    }


    /** Test whether arrays <i>a</i> and <i>b</i> are equal using an epsilon test.
	@param a float array
	@param b float array
	@param eps threshold for epsilon test */
    public static final boolean equal( final float [] a, final float [] b, final float eps ) {
	if( a == b )
	    return true;

	if( a == null || b == null )
	    return false;

	if( a.length != b.length )
	    return false;

	final int length = a.length;

	for( int i=0; i<length; i++ )
	    if( Math.abs( a[i] - b[i] ) > eps )
		return false;

	return true;
    }

    /** Test whether arrays <i>a</i> and <i>b</i> are equal using an epsilon test.
	{@link ArrayUtility#FLOAT_EPS}
	@param a float array
	@param b float array */
    public static final boolean equal( final float [] a, final float [] b ) {
	return equal( a, b, FLOAT_EPS );
    }


    /** Returns copy of <i>source</i> which is the <i>target</i> if it is not
	<i>null</i> and has the right size. */
    public static final float [] copy( final float [] source, final float [] target ) {
	if( target == null || target.length != source.length )
	    return (float[])source.clone();

	System.arraycopy( source, 0, target, 0, source.length );

	return target;
    }

    /** Just returns <i>target</i> if its length equals <i>size</i>.
	Otherwise, this includes <i>target</i> is null, it creates
	an array of length <i>size</i> and returns this instead. */
    public static final float [] size( final float [] target, final int size ) {
	return target != null && target.length == size ? target : new float[size];
    }

    /** Like {@link ArrayUtility#size(float[],int)} but copies also the contents of
	the <i>target</i> if nessecary. */
    public static final float [] resize( final float [] target, final int size ) {
	if( target != null && target.length == size )
	    return target;

	float [] newTarget = new float [size];

	System.arraycopy( target, 0, newTarget, 0, Math.min( size, target.length ) );

	return newTarget;
    }

    /** Returns an array containing <i>numOfTimes</i> copies of the
	contens of <i>values</i>.
	The method uses <i>target</i> if it is not <code>null</code> and has the right size. */
    public static final float [] unrole( float [] target,
					  final float [] values, final int numOfTimes ) {
	int length = values.length * numOfTimes;

	if( target == null || target.length != values.length * numOfTimes )
	    target = new float[ length ];

	System.arraycopy( values, 0, target, 0, values.length );

	int i=values.length;
	for( ; 2*i < length; i *= 2 )
	    System.arraycopy( target, 0, target, i, i );

	System.arraycopy( target, 0, target, i, length - i );

	return target;
    }

    /** Retuns an array containing the contents of <i>con</i> and
	<i>cat</i>.
	The method uses <i>target</i> if it is not <code>null</code> and has the right size. */
    public static final float [] concat( float [] target,
					  final float [] con, final float [] cat ) {
	if( target == null || target.length != con.length + cat.length )
	    target = new float[ con.length + cat.length ];

	System.arraycopy( con, 0, target, 0,          con.length );
	System.arraycopy( cat, 0, target, con.length, cat.length );

	return target;
    }

    /** Fills <i>target</i> with <i>value</i>. */
    public static final void fill( final float [] target, final float value ) {
	fill( target, 0, target.length, value );
    }

    /** Fills <i>target</i> from <i>postion</i> to <i>postion+length-1</i>
	with <i>value</i>. */
    public static final void fill( final float [] target, final int position,
				   final int length, final float value ) {
	if( length < 1 ) return;

	target[position + 0]=value;
	if( length < 2 ) return;
	target[position + 1]=value;
	if( length < 3 ) return;
	target[position + 2]=value;
	if( length < 4 ) return;
	target[position + 3]=value;
	if( length < 5 ) return;
	target[position + 4]=value;

	int i=5;
	for( ; 2*i < length; i *= 2 )
	    System.arraycopy( target, position, target, position + i, i );

	System.arraycopy( target, position, target, position + i, length - i );
    }


    /** Returns the sum of <i>target</i>. */
    public static final float sum( final float [] target ) {
	return sum( target, 0, target.length );
    }

    /** Returns the sum of <i>target</i> from <i>postion</i> to <i>postion+length-1</i>. */
    public static final float sum( final float [] target,
				       final int position, final int length ) {
	float sum = 0;
	for( int i=position; i<position+length; i++ )
	    sum += target[i];
	return sum;
    }

    /** Returns the minimum of <i>target</i>. */
    public static final float min( final float [] target ) {
	return min( target, 0, target.length );
    }

    /** Returns the minimum of <i>target</i> from <i>postion</i> to <i>postion+length-1</i>. */
    public static final float min( final float [] target,
				       final int position, final int length ) {
	float min = target[position];
	for( int i=position; i<position+length; i++ )
	    if( min > target[i] )
		min = target[i];
	return min;
    }


    /** Returns the maximum of <i>target</i>. */
    public static final float max( final float [] target ) {
	return max( target, 0, target.length );
    }

    /** Returns the maximum of <i>target</i> from <i>postion</i> to <i>postion+length-1</i>. */
    public static final float max( final float [] target,
				       final int position, final int length ) {
	float max = target[position];
	for( int i=position; i<position+length; i++ )
	    if( max > target[i] )
		max = target[i];
	return max;
    }



    /** Test whether arrays <i>a</i> and <i>b</i> are equal.
	@param a int array
	@param b int array */
    public static final boolean equal( final int [] a, final int [] b ) {
	if( a == b )
	    return true;

	if( a == null || b == null )
	    return false;

	if( a.length != b.length )
	    return false;

	final int length = a.length;

	for( int i=0; i<length; i++ )
	    if( a[i] != b[i] )
		return false;

	return true;
    }

    /** Returns copy of <i>source</i> which is the <i>target</i> if it is not
	<i>null</i> and has the right size. */
    public static final int [] copy( final int [] source, final int [] target ) {
	if( target == null || target.length != source.length )
	    return (int[])source.clone();

	System.arraycopy( source, 0, target, 0, source.length );

	return target;
    }

    /** Just returns <i>target</i> if its length equals <i>size</i>.
	Otherwise, this includes <i>target</i> is null, it creates
	an array of length <i>size</i> and returns this instead. */
    public static final int [] size( final int [] target, final int size ) {
	return target != null && target.length == size ? target : new int[size];
    }

    /** Like {@link ArrayUtility#size(int[],int)} but copies also the contents of
	the <i>target</i> if nessecary. */
    public static final int [] resize( final int [] target, final int size ) {
	if(  target != null && target.length == size )
	    return target;

	int [] newTarget = new int [size];

	System.arraycopy( target, 0, newTarget, 0, Math.min( size, target.length ) );

	return newTarget;
    }

    /** Returns an array containing <i>numOfTimes</i> copies of the
	contens of <i>values</i>.
	The method uses <i>target</i> if it is not <code>null</code> and has the right size. */
    public static final int [] unrole( int [] target,
					  final int [] values, final int numOfTimes ) {
	int length = values.length * numOfTimes;

	if( target == null || target.length != values.length * numOfTimes )
	    target = new int[ length ];

	System.arraycopy( values, 0, target, 0, values.length );

	int i=values.length;
	for( ; 2*i < length; i *= 2 )
	    System.arraycopy( target, 0, target, i, i );

	System.arraycopy( target, 0, target, i, length - i );

	return target;
    }

    /** Retuns an array containing the contents of <i>con</i> and
	<i>cat</i>.
	The method uses <i>target</i> if it is not <code>null</code> and has the right size. */
    public static final int [] concat( int [] target,
					  final int [] con, final int [] cat ) {
	if( target.length != con.length + cat.length )
	    target = new int[ con.length + cat.length ];

	System.arraycopy( con, 0, target, 0,          con.length );
	System.arraycopy( cat, 0, target, con.length, cat.length );

	return target;
    }

    /** Fills <i>target</i> with <i>value</i>. */
    public static final void fill( final int [] target, final int value ) {
	fill( target, 0, target.length, value );
    }

    /** Fills <i>target</i> from <i>postion</i> to <i>postion+length-1</i>
	with <i>value</i>. */
    public static final void fill( final int [] target, final int position,
				   final int length, final int value ) {
	if( length < 1 ) return;

	target[position + 0]=value;
	if( length < 2 ) return;
	target[position + 1]=value;
	if( length < 3 ) return;
	target[position + 2]=value;
	if( length < 4 ) return;
	target[position + 3]=value;
	if( length < 5 ) return;
	target[position + 4]=value;

	int i=5;
	for( ; 2*i < length; i *= 2 )
	    System.arraycopy( target, position, target, position + i, i );

	System.arraycopy( target, position, target, position + i, length - i );
    }

    /** Returns the sum of <i>target</i>. */
    public static final int sum( final int [] target ) {
	return sum( target, 0, target.length );
    }

    /** Returns the sum of <i>target</i> from <i>postion</i> to <i>postion+length-1</i>. */
    public static final int sum( final int [] target,
				       final int position, final int length ) {
	int sum = 0;
	for( int i=position; i<position+length; i++ )
	    sum += target[i];
	return sum;
    }

    /** Returns the minimum of <i>target/<i>. */
    public static final int min( final int [] target ) {
	return min( target, 0, target.length );
    }

    /** Returns the minimum of <i>target</i> from <i>postion</i> to <i>postion+length-1</i>. */
    public static final int min( final int [] target,
				       final int position, final int length ) {
	int min = target[position];
	for( int i=position; i<position+length; i++ )
	    if( min > target[i] )
		min = target[i];
	return min;
    }


    /** Returns the maximum of <i>target</i>. */
    public static final int max( final int [] target ) {
	return max( target, 0, target.length );
    }

    /** Returns the maximum of <i>target</i> from <i>postion</i> to <i>postion+length-1</i>. */
    public static final int max( final int [] target,
				       final int position, final int length ) {
	int  max = target[position];
	for( int i=position; i<position+length; i++ )
	    if( max > target[i] )
		max = target[i];
	return max;
    }


}
