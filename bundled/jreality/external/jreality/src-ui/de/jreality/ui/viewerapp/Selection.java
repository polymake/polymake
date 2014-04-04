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

import java.util.ArrayList;
import java.util.Collections;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.List;
import java.util.ListIterator;

import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SceneGraphNode;
import de.jreality.scene.SceneGraphPath;
import de.jreality.scene.data.AttributeEntity;
import de.jreality.scene.tool.Tool;

/**
 * Represents selections in a displayed scene. A selection is a list of objects representing a directed path 
 * from a scene graph's root down to some node in the scene graph or arbitrary objects which are somehow linked to 
 * nodes of the scene graph (e.g. tools or attribute entities visible in the scene graph's tree view).
 * <p>
 * In particular, every selection starts with a {@link SceneGraphPath}.
 * Adding an object which is not an instance of SceneGraphNode to the selection's path defines the end of the contained
 * SceneGraphPath, i.e any objects added subsequently are not treated as part of the SceneGraphPath.
 * <p>
 * Selections can be constructed from an existing SceneGraphPaths by using {@link Selection#Selection(SceneGraphPath)}
 * and truncated to a SceneGraphPath using {@link Selection#getSGPath()}.
 * 
 * @author msommer
 */
public class Selection {

	protected SceneGraphPath sgPath;
	protected LinkedList<Object> tail;

	/** Default constructor, starts with an empty selection path */
	public Selection() {
		sgPath = new SceneGraphPath();
		tail = new LinkedList<Object>();
	}

	/** Copy constructor */
	public Selection(Selection s) {
		this(s.sgPath);
		this.tail.addAll(s.tail);
	}
	
	/** Constructs a selection object from the given SceneGraphPath */
	public Selection(SceneGraphPath path) {
		this();
		sgPath = new SceneGraphPath(path);
	}

	/** Clear the selection's path */
	public void clear() {
		sgPath.clear();
		tail.clear();
	}

	/**
	 * When pushing a SceneGraphNode, this method treats it as part of the SceneGraphPath the selection's path starts with
	 * if no other object was pushed before.
	 */
	public final void push(final Object o) {
		if (tail.isEmpty() && o instanceof SceneGraphNode) 
			sgPath.push((SceneGraphNode)o);
		else tail.add(o);
	}
	
	public final void pop() {
		if (tail.isEmpty()) sgPath.pop();
		else tail.removeLast();
	}
	
	/**
	 * Get the first element of the selection's path.<br>
	 * Note that selections always start with a {@link SceneGraphPath}.
	 * @return first SceneGraphNode of the contained path.
	 */
	public SceneGraphNode getFirstElement() {
		return sgPath.getFirstElement();
	}

//	public double[] getMatrix(double[] aMatrix, int begin, int end) {
//	return sgPath.getMatrix(aMatrix, begin, end);
//	}
//	public double[] getMatrix(double[] aMatrix, int begin) {
//	return sgPath.getMatrix(aMatrix, begin);
//	}
//	public double[] getMatrix(double[] aMatrix) {
//	return sgPath.getMatrix(aMatrix);
//	}


//	public double[] getInverseMatrix(double[] aMatrix, int begin, int end) {
//		return sgPath.getInverseMatrix(aMatrix, begin, end);
//	}
//	public double[] getInverseMatrix(double[] invMatrix, int begin) {
//		return sgPath.getInverseMatrix(invMatrix, begin);
//	}
//	public double[] getInverseMatrix(double[] invMatrix) {
//	return sgPath.getInverseMatrix(invMatrix);
//	}

//	public boolean startsWith(Selection potentialPrefix) {
//	if (getLength() < potentialPrefix.getLength()) return false;
//	Iterator i1 = iterator();
//	Iterator i2 = potentialPrefix.iterator();
//	while(i2.hasNext())
//	if (i1.next() != i2.next()) return false;
//	return true;
//	}

	
	/**
	 * Truncates the selection to SceneGraphNodes.
	 * @return the contained scene graph path
	 */
	public SceneGraphPath getSGPath() {
		return sgPath;
	}


	public SceneGraphComponent getLastComponent() {
		return sgPath.getLastComponent();
	}


	public SceneGraphNode getLastNode() {
		return sgPath.getLastElement();
	}


	public Object getLastElement() {
		if (tail.isEmpty()) return getLastNode();
		return tail.getLast();
	}


	/**
	 * Returns true iff the selection corresponds to a SceneGraphPath, 
	 * i.e. consists of SceneGraphNodes.
	 */
	public boolean isSGPath() {
		return tail.isEmpty();
	}
	
	
	public int getLength() {
		return (sgPath.getLength() + tail.size());
	}


	@Override
	public int hashCode() {
		int result = 1;
		for (Object element : toList())
			result = 31 * result + element.hashCode();
		return result;
	}


	@Override
	public boolean equals(Object s) {
		if (s instanceof Selection)
			return isEqual((Selection) s);
		return false;
	}


	public boolean isEqual(Selection anotherSelection) {

		if (anotherSelection == null || getLength() != anotherSelection.getLength())	return false;

		List l1 = toList();
		List l2 = anotherSelection.toList();
		for (int i=0; i<getLength(); ++i)
			if (!l1.get(i).equals(l2.get(i))) return false;

		return true;
	}


	public ListIterator<Object> iterator() {
		return Collections.unmodifiableList(toList()).listIterator();
	}


	public ListIterator<Object> iterator(int start) {
		return Collections.unmodifiableList(toList()).listIterator(start);
	}


	public Iterator reverseIterator() {
		return reverseIterator(getLength());
	}


	public Iterator reverseIterator(int start) {
		final ListIterator iter = iterator(start);
		return new Iterator() {

			public void remove() {
				iter.remove();
			}

			public boolean hasNext() {
				return iter.hasPrevious();
			}

			public Object next() {
				return iter.previous();
			}
		};
	}


	public List<Object> toList() {
		List<Object> list = new ArrayList<Object>(getLength());
		list.addAll(sgPath.toList());
		list.addAll(tail);
		return list;
	}


	@Override
	public String toString() {
		StringBuilder str = new StringBuilder(sgPath.toString());
		for (Object t : tail)	str.append(" : ").append(t.toString());
		return str.toString();
	}

	/** Returns true iff a {@link Tool} was selected */
	public boolean isTool() {
		return (!tail.isEmpty() && tail.getLast() instanceof Tool);
	}
	
	/** Returns the selected tool if the current selection is a {@link Tool}, <code>null</code> otherwise */
	public Tool asTool() {
		if (isTool())
			return (Tool) getLastElement();
		else return null;
	}
	
	/** Returns true iff a {@link SceneGraphComponent} was selected */
	public boolean isComponent() {
		return (isNode() && getLastNode() instanceof SceneGraphComponent);
	}
	
	/** Returns the selected component if the current selection is a {@link SceneGraphComponent}, <code>null</code> otherwise */
	public SceneGraphComponent asComponent() {
		if (isComponent())
			return sgPath.getLastComponent();
		else return null;
	}
	
	/** Returns true iff a {@link SceneGraphNode} was selected */
	public boolean isNode() {
		return (tail.isEmpty() && sgPath.getLength()!=0);
	}
	
	/** Returns the selected node if the current selection is a {@link SceneGraphNode}, <code>null</code> otherwise */
	public SceneGraphNode asNode() {
		if (isNode())
			return getLastNode();
		else return null;
	}
	
	/** Returns true iff an {@link AttributeEntity} was selected */
	public boolean isEntity() {
		return (!tail.isEmpty() && tail.getLast() instanceof AttributeEntity);
	}
	
	/** Returns the selected entity if the current selection is an {@link AttributeEntity}, <code>null</code> otherwise */
	public AttributeEntity asEntity() {
		if (isEntity())
			return (AttributeEntity) getLastElement();
		else return null;
	}
	
}