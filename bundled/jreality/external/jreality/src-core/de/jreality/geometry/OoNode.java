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


package de.jreality.geometry;

import java.util.HashSet;
import java.util.Iterator;
import java.util.Set;

import de.jreality.scene.data.DataList;
import de.jreality.scene.data.StorageModel;

class OoNode {

	interface IsUpdateCounter {
		public long getUpdateCount();
	}
	
	private IsUpdateCounter updateCounter;
	
	private Set ingr = new HashSet();
	private Set deps = new HashSet();
	
	private Object object;
	
	private Class type;
	
	private String name;
	
	private long counterOfLastUpdate = -1; //System.currentTimeMillis();
	
	private boolean currentlyUpdating = false;
	private boolean currentlyOutdating = false;
	
	private boolean outOfDate = true;
	
	UpdateMethod updateMethod = null;
	
	//static Logger outdateLog = Logger.getAnonymousLogger();//.getLogger("de.jreality.geometry.factory.outdate");
	//static Logger updateLog  = Logger.getAnonymousLogger();//.getLogger("de.jreality.geometry.factory.update");
	{
		//updateLog.setUseParentHandlers(false);
		//outdateLog.setUseParentHandlers(false);
		//outdateLog.setLevel(Level.OFF);
		//updateLog.setLevel(Level.OFF);
	}
	
	public OoNode( String name, IsUpdateCounter updateCounter  ) {
		this( name, (Class)null, updateCounter );
	}
	
	public OoNode( String name, Class type, IsUpdateCounter updateCounter  ) {
		this.updateCounter = updateCounter; 
		setObject(object);
		this.type = type;
		setName( name );
	}

	public OoNode( Object object, String name, IsUpdateCounter updateCounter ) {
		this( object, object.getClass(), name, updateCounter );
	}
	
	public OoNode( Object object, Class type, String name, IsUpdateCounter updateCounter ) {
		this.updateCounter = updateCounter; 
		setObject(object);
		this.type = type;
		setName( name );
	}
	
	public Object getObject() {
		update();
		return object;
	}
	
	public void setObject( Object object ) {
		if( object != null && type != null && ! type.isAssignableFrom(object.getClass()) )
			throw new IllegalArgumentException( "object of incompatible type" );
		
		if( this.object == object ) //&& this.object.equals( object))  //overhead too big
			return;
		this.object=object;
		outdate();
	}
	
	public String getName() {
		return name;
	}
	
	public void setName( String name ) {
		this.name = name;
	}
	
	public void addIngr( OoNode node ) {
		node.addDeps(this);
//		if( node == this )
//			throw new IllegalArgumentException( "node must not equal this" );
//		ingr.add(node);
//		node.deps.add(this);
	}

	public void removeIngr( OoNode node ) {
		node.removeDeps(this);
	}
	
	public void addDeps( OoNode node ) {
	if( node == this )
			throw new IllegalArgumentException( "node must not equal this" );
		deps.add(node);
		
		if( this.isOutOfDate() )
			node.outdate();
		
		node.ingr.add(this);
	}
	
	public void removeDeps( OoNode node ) {
		if( node == this )
				throw new IllegalArgumentException( "node must not equal this" );
			deps.remove(node);
			node.ingr.remove(this);
		}

	void outdateDeps() {
		for( Iterator iter=deps.iterator(); iter.hasNext(); ) {
			((OoNode)iter.next()).outdate();
		}
	}
	
	public void outdate() {
		if( currentlyOutdating ) {
			throw new IllegalStateException("encounterd loop in update graph: " + name );
		}
		currentlyOutdating = true;
		
		try {
			outOfDate=true;
			outdateDeps();
			
		} finally {
			currentlyOutdating = false;
		}		
	}
	
	public void update() {
		if( !outOfDate )
			return;
		
		if( currentlyUpdating ) {
			throw new IllegalStateException("encounterd loop in update graph: " + name );
		}
		
		currentlyUpdating=true;

		try {
			
			for( Iterator iter=ingr.iterator(); iter.hasNext(); ) {
				((OoNode)iter.next()).update();
			}
		
			if( updateMethod != null ) {
				//updateLog.info(name);
				Object newObject = updateMethod.update( object );
				/*
				if( newObject != null && !newObject.equals(object) || newObject == null && object != null )
					outdateDeps();
				*/
				object = newObject;
			}
			
			//counterOfLastUpdate = System.currentTimeMillis();
			
		} finally {
			currentlyUpdating=false;
		}
		outOfDate=false;
		counterOfLastUpdate = updateCounter.getUpdateCount();
	}

	public void fire() {
		outdate();
		update();
	}
	
	public interface UpdateMethod {
		public Object update( Object object );
	}

	public boolean isOutOfDate() {
		return this.outOfDate;
	}

	public void setUpdateMethod(UpdateMethod method) {
		if( method == updateMethod )
			return;
		updateMethod = method;
		outdate(); //TODO: is this o.k. ?
	}

	public long getCounterOfLastUpdate() {
		return counterOfLastUpdate;
	}
	
	DataList createDataList() {
		Object currentObject = getObject();
		
		if( currentObject==null)
				return null;
		
		if( type.equals(  int[][].class ) ) {
			int[][] array = (int[][])(currentObject);
			return StorageModel.INT_ARRAY_ARRAY.createReadOnly(array);		
		} else if( type == double[].class ) {
			double[] array = (double[])(currentObject);
			return StorageModel.DOUBLE_ARRAY.createReadOnly(array);		
		} else if( type == double[][].class ) {
			double[][] array = (double[][])(currentObject);
			return StorageModel.DOUBLE_ARRAY_ARRAY.createReadOnly(array);		
		} else if( type.equals( String[].class ) ) {
			String[] array = (String[])currentObject;
			return StorageModel.STRING_ARRAY.createReadOnly( array );
		}
		
		throw new IllegalStateException( "do not support type " + type );		
	}
	
	Object converteDataListToArray( DataList dl ) {
		if( type.equals(  int[][].class ) ) {
			return dl.toIntArrayArray(null);			
		} else if( type == double[][].class ) {
			return dl.toDoubleArrayArray(null);		
		} else if( type.equals( String[].class ) ) {
			return dl.toStringArray(null);		
		}
		
		throw new IllegalStateException( "do not support type " + type );	
		
	}
}
