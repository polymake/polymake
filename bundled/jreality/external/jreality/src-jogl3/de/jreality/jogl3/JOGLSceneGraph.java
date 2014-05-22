package de.jreality.jogl3;

import de.jreality.jogl3.geom.JOGLEmptyEntity;
import de.jreality.jogl3.geom.JOGLFaceSetEntity;
import de.jreality.jogl3.geom.JOGLFaceSetInstance;
import de.jreality.jogl3.geom.JOGLLineSetEntity;
import de.jreality.jogl3.geom.JOGLLineSetInstance;
import de.jreality.jogl3.geom.JOGLPointSetEntity;
import de.jreality.jogl3.geom.JOGLPointSetInstance;
import de.jreality.jogl3.geom.JOGLSphereEntity;
import de.jreality.jogl3.geom.JOGLSphereInstance;
import de.jreality.jogl3.light.JOGLDirectionalLightEntity;
import de.jreality.jogl3.light.JOGLDirectionalLightInstance;
import de.jreality.jogl3.light.JOGLPointLightEntity;
import de.jreality.jogl3.light.JOGLPointLightInstance;
import de.jreality.jogl3.light.JOGLSpotLightEntity;
import de.jreality.jogl3.light.JOGLSpotLightInstance;
import de.jreality.scene.Appearance;
import de.jreality.scene.ClippingPlane;
import de.jreality.scene.Cylinder;
import de.jreality.scene.DirectionalLight;
import de.jreality.scene.Geometry;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.IndexedLineSet;
import de.jreality.scene.Light;
import de.jreality.scene.PointLight;
import de.jreality.scene.PointSet;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SceneGraphNode;
import de.jreality.scene.SceneGraphVisitor;
import de.jreality.scene.Sphere;
import de.jreality.scene.SpotLight;
import de.jreality.scene.Transformation;
import de.jreality.scene.proxy.tree.EntityFactory;
import de.jreality.scene.proxy.tree.ProxyTreeFactory;
import de.jreality.scene.proxy.tree.SceneGraphNodeEntity;
import de.jreality.scene.proxy.tree.UpToDateSceneProxyBuilder;

public class JOGLSceneGraph extends UpToDateSceneProxyBuilder {
	
	public boolean lightsChanged = false;
	
	JOGLSceneGraph me = this;
	
	class JOGLEntityFactory extends EntityFactory {
		
		SceneGraphNodeEntity entity;
		
		SceneGraphVisitor creator = new SceneGraphVisitor() {
			@Override
			public void visit(IndexedFaceSet i) {
				entity = new JOGLFaceSetEntity(i);
			}
			@Override
			public void visit(IndexedLineSet g) {
				entity = new JOGLLineSetEntity(g);
			}
			@Override
			public void visit(PointSet p) {
				entity = new JOGLPointSetEntity(p);
			}
			@Override
			public void visit(Transformation t) {
				entity = new JOGLTransformationEntity(t);
			}
			@Override
			public void visit(Appearance a) {
				entity = new JOGLAppearanceEntity(a);
			}
			@Override
			public void visit(SpotLight l) {
				entity = new JOGLSpotLightEntity(l, me);
			}
			@Override
			public void visit(PointLight l) {
				entity = new JOGLPointLightEntity(l, me);
			}
			@Override
			public void visit(DirectionalLight l) {
				entity = new JOGLDirectionalLightEntity(l, me);
			}
			@Override
			public void visit(Sphere s) {
				entity = new JOGLSphereEntity(s);
			};
			@Override
			public void visit(Cylinder c) {
				entity = new JOGLEmptyEntity(c);
			};
			@Override
			public void visit(ClippingPlane c) {
				entity = new JOGLEmptyEntity(c);
			};
			@Override
			public void visit(Geometry n) {
				throw new RuntimeException("unhandled geometry type: "+n.getClass());
			};
		};
		
		public JOGLEntityFactory() {
			setUpdateAppearance(true);
			setUpdateTransformation(true);
			setUpdateGeometry(true);
			setUpdateLight(true);
		}
		
		private SceneGraphNodeEntity create(SceneGraphNode g) {
			entity = null;
			g.accept(creator);
			return entity;
		}
		
		@Override
		protected SceneGraphNodeEntity produceSceneGraphNodeEntity(SceneGraphNode n) {
			entity = create(n);
			if (entity == null) return super.produceSceneGraphNodeEntity(n);
			else return entity;
		}
		@Override
		protected SceneGraphNodeEntity produceGeometryEntity(Geometry g) {
			return create(g);
		}
		@Override
		protected SceneGraphNodeEntity produceAppearanceEntity(Appearance a) {
			return create(a);
		}
		@Override
		protected SceneGraphNodeEntity produceTransformationEntity(
				Transformation t) {
			return create(t);
		}
		@Override
		protected SceneGraphNodeEntity produceLightEntity(
				Light l) {
			return create(l);
		}
	}	
	class JOGLTreeFactory extends ProxyTreeFactory {
		@Override
		public void visit(Sphere s) {
			proxyNode = new JOGLSphereInstance(s);
		}
		@Override
		public void visit(IndexedFaceSet i) {
			proxyNode = new JOGLFaceSetInstance(i);
		}
		@Override
		public void visit(IndexedLineSet g) {
			proxyNode = new JOGLLineSetInstance(g);
		}
		@Override
		public void visit(PointSet p) {
			proxyNode = new JOGLPointSetInstance(p);
		}
		@Override
		public void visit(SceneGraphComponent c) {
			proxyNode = new JOGLSceneGraphComponentInstance(c);
		}
		@Override
		public void visit(SpotLight c) {
			proxyNode = new JOGLSpotLightInstance(c);
		}
		@Override
		public void visit(PointLight c) {
			proxyNode = new JOGLPointLightInstance(c);
		}
		@Override
		public void visit(DirectionalLight c) {
			proxyNode = new JOGLDirectionalLightInstance(c);
		}
		@Override
		public void visit(Appearance a) {
			proxyNode = new JOGLAppearanceInstance(a);
		}
	}
	
	public JOGLSceneGraph(SceneGraphComponent root) {
		super(root);
		setEntityFactory(new JOGLEntityFactory());
		setProxyTreeFactory(new JOGLTreeFactory());
		me = this;
	}

}
