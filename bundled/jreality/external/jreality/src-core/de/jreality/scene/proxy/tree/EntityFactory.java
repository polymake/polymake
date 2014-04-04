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


package de.jreality.scene.proxy.tree;


import de.jreality.scene.Appearance;
import de.jreality.scene.AudioSource;
import de.jreality.scene.Geometry;
import de.jreality.scene.Light;
import de.jreality.scene.SceneGraphNode;
import de.jreality.scene.SceneGraphVisitor;
import de.jreality.scene.Transformation;
import de.jreality.scene.event.AppearanceListener;
import de.jreality.scene.event.AudioListener;
import de.jreality.scene.event.GeometryListener;
import de.jreality.scene.event.LightListener;
import de.jreality.scene.event.TransformationListener;


/**
 *
 * This class produces SceneGraphNodeEntities, based on the
 * desired update behavior for the proxy.
 * 
 * inherit this class and overwrite the methods
 * 
 * <li>produceTransformationEntity
 * <li>produceAppearanceEntity
 * <li>produceGeometryEntity
 * <br>
 * as needed. These are only called if the corresponding update method
 * is set to true, then the Entities are attatched as Listeners to
 * the SceneGraphNode.
 * 
 * @author weissman
 *
 */
public class EntityFactory {

	private SceneGraphNodeEntity produced;

	private boolean updateTransformation;
	private boolean updateAppearance;
	private boolean updateAudioSource;
	private boolean updateGeometry;
	private boolean updateLight;

	public boolean isUpdateLight() {
		return updateLight;
	}
	public void setUpdateLight(boolean updateLight) {
		this.updateLight = updateLight;
	}
	public boolean isUpdateAppearance() {
		return updateAppearance;
	}
	public void setUpdateAppearance(boolean updateAppearance) {
		this.updateAppearance = updateAppearance;
	}
	public boolean isUpdateAudioSource() {
		return updateAudioSource;
	}
	public void setUpdateAudioSource(boolean updateAudioSource) {
		this.updateAudioSource = updateAudioSource;
	}
	public boolean isUpdateGeometry() {
		return updateGeometry;
	}
	public void setUpdateGeometry(boolean updateGeometry) {
		this.updateGeometry = updateGeometry;
	}
	public boolean isUpdateTransformation() {
		return updateTransformation;
	}
	public void setUpdateTransformation(boolean updateTransformation) {
		this.updateTransformation = updateTransformation;
	}

	SceneGraphNodeEntity createEntity(SceneGraphNode node) {
		node.accept(createTraversal);
		return produced;
	}
	private SceneGraphVisitor createTraversal = new SceneGraphVisitor() {
		@Override
		public void visit(Light a) {
			if (updateLight) {
				produced = produceLightEntity(a);
				a.addLightListener((LightListener) produced);
			}
			else super.visit(a);
		}
		public void visit(AudioSource a) {
			if (updateAudioSource) {
				produced = produceAudioSourceEntity(a);
				a.addAudioListener((AudioListener) produced);
			}
			else super.visit(a);
		}
		public void visit(Appearance a) {
			if (updateAppearance) {
				produced=produceAppearanceEntity(a);
				a.addAppearanceListener((AppearanceListener) produced);
			}
			else super.visit(a);
		}
		public void visit(Geometry g) {
			if (updateGeometry) {
				produced=produceGeometryEntity(g);
				g.addGeometryListener((GeometryListener) produced);
			}
			else super.visit(g);
		}
		public void visit(Transformation t) {
			if (updateTransformation) {
				produced=produceTransformationEntity(t);
				t.addTransformationListener((TransformationListener) produced);
			}
			else super.visit(t);
		}
		public void visit(SceneGraphNode n) {
			produced=produceSceneGraphNodeEntity(n);
		}
	};

	public void disposeEntity(SceneGraphNodeEntity entity) {
		produced=entity;
		entity.getNode().accept(disposeTraversal); // remove listeners
		entity.dispose();
	}

	private SceneGraphVisitor disposeTraversal = new SceneGraphVisitor() {
		public void visit(Light l) {
			if (updateLight) {
				l.removeLightListener((LightListener) produced);
			}
		}
		public void visit(Appearance a) {
			if (updateAppearance) {
				a.removeAppearanceListener((AppearanceListener) produced);
			}
		}
		public void visit(Geometry g) {
			if (updateGeometry) {
				g.removeGeometryListener((GeometryListener) produced);
			}
		}
		public void visit(Transformation t) {
			if (updateTransformation) {
				t.removeTransformationListener((TransformationListener) produced);
			}
		}
	};

	protected SceneGraphNodeEntity produceSceneGraphNodeEntity(SceneGraphNode n) {
		return new SceneGraphNodeEntity(n);
	}
	/**
	 * this method must return a SceneGraphNodeEntity that
	 * implements TransformationListener!
	 */
	protected SceneGraphNodeEntity produceTransformationEntity(Transformation t) {
		throw new IllegalStateException("not implemented");
	}
	/**
	 * this method must return a SceneGraphNodeEntity that
	 * implements AppearanceListener!
	 */
	protected SceneGraphNodeEntity produceAppearanceEntity(Appearance a) {
		throw new IllegalStateException("not implemented");
	}
	/**
	 * this method must return a SceneGraphNodeEntity that
	 * implements GeometryListener!
	 */
	protected SceneGraphNodeEntity produceGeometryEntity(Geometry g) {
		throw new IllegalStateException("not implemented");
	}
	/**
	 * this method must return a SceneGraphNodeEntity that
	 * implements AudioListener!
	 */
	protected SceneGraphNodeEntity produceAudioSourceEntity(AudioSource g) {
		throw new IllegalStateException("not implemented");
	}
	/**
	 * this method must return a SceneGraphNodeEntity that
	 * implements LightListener!
	 */
	protected SceneGraphNodeEntity produceLightEntity(Light l) {
		throw new IllegalStateException("not implemented");
	}
}
