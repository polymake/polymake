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


package de.jreality.scene.pick;

import de.jreality.scene.SceneGraphPath;

/**
 * @author brinkman
 *
 * TODO document PickResult
 * TODO add support for picking vertices, edges, faces, etc.
 */
public interface PickResult {
  
  public static final int PICK_TYPE_POINT=4;
  public static final int PICK_TYPE_LINE=2;
  public static final int PICK_TYPE_FACE=1;
  public static final int PICK_TYPE_OBJECT=0;
  
  
	public SceneGraphPath getPickPath();
	
	/**
	 * 
	 * @return pick point in world coordinates
	 */
	public double[] getWorldCoordinates();
	
	/**
	 * 
	 * @return pick point in object coordinates
	 */
	public double[] getObjectCoordinates();

  /**
   * returns the index of the picked face/edge/point
   * @return the index or -1 if not available
   */
  public int getIndex();
  public int getSecondaryIndex();
  
  /**
   * returns if the type of the pick:
   * - PICK_TYPE_OBJECT
   * - PICK_TYPE_FACE
   * - PICK_TYPE_LINE
   * - PICK_TYPE_POINT
   * @return
   */
  public int getPickType();
  
  /**
   * returns texture coordinates if available.
   * @return the coordinates of null.
   */
  public double[] getTextureCoordinates();
  
 }
