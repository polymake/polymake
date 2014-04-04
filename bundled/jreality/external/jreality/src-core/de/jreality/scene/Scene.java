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


package de.jreality.scene;

import java.util.Collection;
import java.util.Comparator;
import java.util.HashSet;
import java.util.TreeSet;

import de.jreality.geometry.GeometryFactory;

/**
 * A utility class providing static methods 
 * for applying a set of changes simultaneously to an element of the
 * scene graph. It minimizes the amount of locking that is required to do this.
 */
public class Scene
{
  private Scene() {}

  private static final ThreadLocal ownedReadLocks=new ThreadLocal() {
    protected Object initialValue() {
      return new HashSet();
    }
  };
  private static final Comparator CANONICAL=new Comparator() {
    public int compare(Object o1, Object o2) {
      return System.identityHashCode(o1)-System.identityHashCode(o2);
    }
  };
  /**
   * Perform a piece of code to query a {@link SceneGraphNode} in a read-locked
   * state of the node. The code is encapsulated in a {@link Runnable} and can
   * safely assume that different query methods on that node return valid values
   * representing the same unmodified state.
   * @see #executeReader(Collection,Runnable)
   */
  public static void executeReader(SceneGraphNode toRead, Runnable reader) {
//    executeReader(Collections.singleton(toRead), reader);
    //HOTFIX, until executeReader(Collection, Runnable) works
    toRead.startReader();
    try {
      reader.run();
    } finally {
      toRead.finishReader();
    }
  }
  /**
   * <b style="text-color:red">Not implemented yet.</b>
   */
  public static void executeReader(Collection toRead, Runnable reader) {
    if (true) throw new UnsupportedOperationException("not yet implemented");
    HashSet owned=(HashSet)ownedReadLocks.get();
    if(owned.containsAll(toRead)) {//no need for any protection
      reader.run();
      return;
    }
    TreeSet toLock=new TreeSet(toRead);
    toLock.removeAll(owned);
    //TODO: continue
  }
  /**
   * Perform a piece of code to manipulate a {@link SceneGraphNode} in a
   * write-locked state of the node. The code is encapsulated in a
   * {@link Runnable} and no other code can get a read-lock while it's
   * {@link Runnable#run() run} method is being executed. So no-one will
   * read an in-between state.
   * @see #executeWriter(Collection,Runnable)
   */
  public static void executeWriter(SceneGraphNode toWriteIn, Runnable writer) {
//    executeWriter(Collections.singleton(toWriteIn), writer);
    //HOTFIX, until executeWriter(Collection, Runnable) works
    toWriteIn.startWriter();
    try {
      writer.run();
    } finally {
      toWriteIn.finishWriter();
    }
  }
  /**
   * <b style="text-color:red">Not implemented yet.</b>
   */
  public static void executeWriter(Collection toRead, Runnable writer) {
    if (true) throw new UnsupportedOperationException("not yet implemented");
  }
  
  /**
   * not yet tested...
   * @param factories
   */
  public static void updateFactories(GeometryFactory... factories) {
	  for (GeometryFactory factory : factories) {
		  factory.getGeometry().startWriter();
	  }
	  try {
		  for (GeometryFactory factory : factories) {
			  factory.update();
		  }
	  } finally {
		  for (GeometryFactory factory : factories) {
			  factory.getGeometry().finishWriter();
		  }
	  }
  }
}
