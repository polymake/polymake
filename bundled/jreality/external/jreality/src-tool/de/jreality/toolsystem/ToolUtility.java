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


package de.jreality.toolsystem;

import de.jreality.math.Matrix;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SceneGraphPath;
import de.jreality.scene.tool.InputSlot;
import de.jreality.scene.tool.ToolContext;

public class ToolUtility {

  private ToolUtility() {}
  
  public static Matrix worldToAvatar(ToolContext tc, Matrix worldMatrix) {
    Matrix world2avatar = new Matrix(tc.getTransformationMatrix(InputSlot.getDevice("AvatarTransformation")));
    world2avatar.invert();
    world2avatar.multiplyOnRight(worldMatrix);
    return world2avatar;
  }
  public static double[] worldToAvatar(ToolContext tc, double[] worldVector) {
    Matrix world2avatar = new Matrix(tc.getTransformationMatrix(InputSlot.getDevice("AvatarTransformation")));
    world2avatar.invert();
    return world2avatar.multiplyVector(worldVector);
  }

  public static Matrix avatarToWorld(ToolContext tc, Matrix localMatrix) {
    Matrix avatar2world = new Matrix(tc.getTransformationMatrix(InputSlot.getDevice("AvatarTransformation")));
    avatar2world.multiplyOnRight(localMatrix);
    return avatar2world;
  }
  public static double[] avatarToWorld(ToolContext tc, double[] localVector) {
    Matrix avatar2world = new Matrix(tc.getTransformationMatrix(InputSlot.getDevice("AvatarTransformation")));
    return avatar2world.multiplyVector(localVector);
  }

  public static Matrix worldToLocal(ToolContext tc, Matrix worldMatrix) {
    return worldToLocal(tc.getRootToLocal(), worldMatrix);//worldToLocal(tc.getRootToLocal(), worldMatrix);
  }

  public static double[] worldToLocal(ToolContext tc, double[] worldVector) {
    return worldToLocal(tc.getRootToLocal(), worldVector);
  }
  public static Matrix worldToTool(ToolContext tc, Matrix worldMatrix) {
    return worldToLocal(tc.getRootToToolComponent(), worldMatrix);
  }
  public static double[] worldToTool(ToolContext tc, double[] worldVector) {
    return worldToLocal(tc.getRootToToolComponent(), worldVector);
  }

  public static Matrix localToWorld(ToolContext tc, Matrix localMatrix) {
    return localToWorld(tc.getRootToLocal(), localMatrix);
  }
  public static double[] localToWorld(ToolContext tc, double[] localVector) {
    return localToWorld(tc.getRootToLocal(), localVector);
  }
  
  public static Matrix toolToWorld(ToolContext tc, Matrix toolMatrix) {
    return localToWorld(tc.getRootToToolComponent(), toolMatrix);
  }
  public static double[] toolToWorld(ToolContext tc, double[] toolVector) {
    return localToWorld(tc.getRootToToolComponent(), toolVector);
  }
  
  public static Matrix worldToLocal(SceneGraphPath rootToLocal, Matrix worldMatrix) {
    Matrix world2local = new Matrix();
    rootToLocal.getInverseMatrix(world2local.getArray());
    world2local.multiplyOnRight(worldMatrix);
    return world2local;
  }
  public static double[] worldToLocal(SceneGraphPath rootToLocal, double[] worldVector) {
    Matrix world2local = new Matrix();
    rootToLocal.getInverseMatrix(world2local.getArray());
    return world2local.multiplyVector(worldVector);
  }
  public static Matrix localToWorld(SceneGraphPath rootToLocal, Matrix localMatrix) {
    Matrix local2world = new Matrix();
    rootToLocal.getMatrix(local2world.getArray());
    local2world.multiplyOnRight(localMatrix);
    return local2world;
  }
  public static double[] localToWorld(SceneGraphPath rootToLocal, double[] localVector) {
    Matrix local2world = new Matrix();
    rootToLocal.getMatrix(local2world.getArray());
    return local2world.multiplyVector(localVector);
  }
  public static void attachTimer(javax.swing.Timer timer, ToolContext tc) {
	  if (timer instanceof de.jreality.tools.Timer) {
		de.jreality.tools.Timer tt = (de.jreality.tools.Timer) timer;
		tt.attach(tc);
	}
  }
//  public static void attachTimer(javax.swing.Timer timer, ToolSystemViewer tc) {
//	  if (timer instanceof de.jreality.tools.Timer) {
//		de.jreality.tools.Timer tt = (de.jreality.tools.Timer) timer;
//		tt.attach(tc);
//	}
//  }
  public static void attachTimer(javax.swing.Timer timer, SceneGraphComponent tc) {
	  if (timer instanceof de.jreality.tools.Timer) {
		de.jreality.tools.Timer tt = (de.jreality.tools.Timer) timer;
		tt.attach(tc);
	}
  }

}
