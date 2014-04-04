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

import java.util.LinkedList;
import java.util.List;

import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SceneGraphNode;
import de.jreality.scene.SceneGraphPath;
import de.jreality.scene.SceneGraphVisitor;

/**
 * This visitor traverses a scene graph searching for {@link Path all paths} betweenthe given 
 * {@link SceneGraphComponent} and the given {@link SceneGraphNode}.
 * 
 * TODO: make this a collector, put methods in SceneGraphUtility (get all paths as well)
 * 
 * TODO: return list of ALL paths
 * 
 * make singleton 
 * @author Charles Gunn
 *
*/
public class PathCollector extends SceneGraphVisitor {

    SceneGraphComponent root;
    SceneGraphPath currentPath = new SceneGraphPath();
    LinkedList<SceneGraphPath> collectedPaths = new LinkedList<SceneGraphPath>();
    Matcher matcher;
    
	  public PathCollector(Matcher matcher, SceneGraphComponent root)	{
	  	this.matcher=matcher;
	  	this.root=root;
	  }

	   public List<SceneGraphPath> visit()	{
		  visit(root);
		  return collectedPaths;
	  }
	  
	  public void visit(SceneGraphNode m) {
		  	currentPath.push(m);
			if (currentPath.getLength() > 0 && matcher.matches(currentPath)) {
			  collectedPaths.add((SceneGraphPath) currentPath.clone());
			}
			currentPath.pop();
	  }
	
	  public void visit(SceneGraphComponent c) {
      super.visit(c);
  	  currentPath.push(c);
		  c.childrenAccept(this);
		  currentPath.pop();
	}
    
  public interface Matcher {
     boolean matches(SceneGraphPath p);
  };

}
