package de.jreality.jogl3;

import java.util.List;

import de.jreality.jogl3.geom.JOGLGeometryEntity;
import de.jreality.jogl3.geom.JOGLGeometryInstance;
import de.jreality.jogl3.light.JOGLLightCollection;
import de.jreality.jogl3.light.JOGLLightEntity;
import de.jreality.jogl3.light.JOGLLightInstance;
import de.jreality.jogl3.optimization.RenderableUnitCollection;
import de.jreality.math.Rn;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.proxy.tree.SceneTreeNode;
import de.jreality.shader.CommonAttributes;
import de.jreality.shader.ShaderUtility;

public class JOGLSceneGraphComponentInstance extends SceneTreeNode {

	protected JOGLSceneGraphComponentInstance(SceneGraphComponent cmp) {
		super(cmp);
	}
	
	//have to do this, as we have no means in determining whether an
	//AppearanceEntity could be visited another time during the scene
	//graph traversal
	void setAppearanceEntitiesUpToDate(){
		
		JOGLAppearanceInstance app = (JOGLAppearanceInstance) getAppearanceTreeNode();
		if(app != null){
			((JOGLAppearanceEntity)(app.getEntity())).dataUpToDate = true;
		}
		
		for (SceneTreeNode child : getChildren()) {
			if (!child.isComponent())
				continue;
			JOGLSceneGraphComponentInstance childInstance = (JOGLSceneGraphComponentInstance) child;
			childInstance.setAppearanceEntitiesUpToDate();
		}
	}
	
	public class RenderableObject{
		public RenderableObject(JOGLGeometryInstance geom, JOGLRenderState state){
			this.state = state;
			this.geom = geom;
		}
		public JOGLRenderState state;
		public JOGLGeometryInstance geom;
		public void render(int width, int height){
			geom.render(state, width, height);
		}
		public void renderDepth(int width, int height){
			geom.renderDepth(state, width, height);
		}
		public void addOneLayer(int width, int height){
			float alpha = 1 - (float)geom.eap.getAttribute(ShaderUtility.nameSpace("polygonShader", CommonAttributes.TRANSPARENCY), CommonAttributes.TRANSPARENCY_DEFAULT);
			geom.addOneLayer(state, width, height, alpha);
		}
	}
	
	void collectTranspAndNonTransparent(JOGLRenderState parentState, RenderableUnitCollection ruc, List<RenderableObject> transpObjects) {
		JOGLAppearanceInstance app = (JOGLAppearanceInstance) getAppearanceTreeNode();
		boolean upToDate = false;
		if(app != null){
			if (((JOGLAppearanceEntity)app.getEntity()).dataUpToDate && parentState.appearanceUpToDate){
				
				upToDate = true;
			}
		}
		if(app == null && parentState.appearanceUpToDate){
			upToDate = true;
		}
		JOGLRenderState state = new JOGLRenderState(parentState, getTrafo());
		state.appearanceUpToDate = upToDate;
		
		//if two scenegraph nodes on the same scene graph path reference
		//to the same local light, then the light is duplicated and therefore its
		//intensity is practically multiplied. There would be no other possible meaning in
		//referencing a local light twice on the same scene graph path.
		SceneTreeNode maybelight = this.getLightTreeNode();
		if(maybelight instanceof JOGLLightInstance){
			JOGLLightInstance light = (JOGLLightInstance) maybelight;
			if(light != null){
				JOGLLightEntity lightEntity = (JOGLLightEntity) light.getEntity();
				if(!lightEntity.isGlobal()){
					state.addLocalLight(light);
				}
			}
		}
		
		SceneTreeNode gtn =  getGeometryTreeNode();
		
		if (gtn instanceof JOGLGeometryInstance) {
		
			JOGLGeometryInstance geom = (JOGLGeometryInstance) gtn;
			if (geom != null) {
				JOGLGeometryEntity geomEntity = (JOGLGeometryEntity) geom.getEntity();
				if(!geomEntity.dataUpToDate){
					boolean changedLength = geomEntity.updateData(state.getGL());
					geom.updateAppearance(this.toPath(), state.getGL(), false, changedLength, true);
					geomEntity.dataUpToDate = true;
				}
				if(!state.appearanceUpToDate)
					geom.updateAppearance(this.toPath(), state.getGL(), true, false, false);
				//geom.eap = EffectiveAppearance.create(this.toPath());
				//rather update only when appearance has changed.
				//PolygonShader.setFromEffectiveAppearance(EffectiveAppearance.create(this.toPath()), CommonAttributes.POLYGON_SHADER);
				boolean transpEnabled = (Boolean)geom.eap.getAttribute(CommonAttributes.TRANSPARENCY_ENABLED, new Boolean(false));
				if(!transpEnabled){
					ruc.add(new RenderableObject(geom, state));
				}else{
					transpObjects.add(new RenderableObject(geom, state));
				}
				//geom.render(state);
			}
		}
		
		for (SceneTreeNode child : getChildren()) {
			if (!child.isComponent())
				continue;
			JOGLSceneGraphComponentInstance childInstance = (JOGLSceneGraphComponentInstance) child;
			SceneGraphComponent sgc = (SceneGraphComponent)child.getNode();
			if(sgc.isVisible())
				childInstance.collectTranspAndNonTransparent(state, ruc, transpObjects);
		}
	}
	private boolean hasLights = false;
	//this method collects global lights and updates the data in every
	//(also local) lights
	public boolean collectGlobalLights(double[] trafo, JOGLLightCollection collection, boolean visitAll) {
		if(!visitAll){
			if(!hasLights){
			}else{
				double[] newMatrix = new double[16];
				
				double[] matrix = getTrafo();
				if (matrix != null)
					Rn.times(newMatrix, trafo, matrix);
				else
					System.arraycopy(trafo, 0, newMatrix, 0, 16);
				
				JOGLLightInstance light = (JOGLLightInstance)getLightTreeNode();
				
				if (light != null) {
					JOGLLightEntity lightEntity = (JOGLLightEntity) light.getEntity();
					lightEntity.updateData();
					//System.out.println(light.getEntity().getNode().getName() + light.getEntity().getNode().getClass());
					//copy transformation to light.trafo
					light.trafo = newMatrix;
					if(lightEntity.isGlobal())
						light.addToList(collection);
				}
				
				for (SceneTreeNode child : getChildren()) {
					if (!child.isComponent()) continue;
					JOGLSceneGraphComponentInstance childInstance = (JOGLSceneGraphComponentInstance) child;
					SceneGraphComponent sgc = (SceneGraphComponent)child.getNode();
					if(sgc.isVisible()){
						childInstance.collectGlobalLights(newMatrix, collection, visitAll);
					}
				}
			}
			return false;
		}else{
			double[] newMatrix = new double[16];
			
			double[] matrix = getTrafo();
			if (matrix != null)
				Rn.times(newMatrix, trafo, matrix);
			else
				System.arraycopy(trafo, 0, newMatrix, 0, 16);
			
			JOGLLightInstance light = (JOGLLightInstance)getLightTreeNode();
			
			hasLights = false;
			if (light != null) {
				hasLights = true;
				JOGLLightEntity lightEntity = (JOGLLightEntity) light.getEntity();
				lightEntity.updateData();
				//System.out.println(light.getEntity().getNode().getName() + light.getEntity().getNode().getClass());
				//copy transformation to light.trafo
				light.trafo = newMatrix;
				if(lightEntity.isGlobal())
					light.addToList(collection);
			}
			
			for (SceneTreeNode child : getChildren()) {
				if (!child.isComponent()) continue;
				JOGLSceneGraphComponentInstance childInstance = (JOGLSceneGraphComponentInstance) child;
				SceneGraphComponent sgc = (SceneGraphComponent)child.getNode();
				if(sgc.isVisible()){
					boolean temp = childInstance.collectGlobalLights(newMatrix, collection, visitAll);
					if(temp)
						hasLights = true;
				}
			}
			return hasLights;
		}
	}
	
	private double[] getTrafo() {
		SceneTreeNode tn = getTransformationTreeNode();
		if (tn != null) {
			JOGLTransformationEntity te = (JOGLTransformationEntity) tn.getEntity();
			return te.matrix;
		}
		return null;
	}
}
