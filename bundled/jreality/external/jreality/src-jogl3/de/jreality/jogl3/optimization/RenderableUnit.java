package de.jreality.jogl3.optimization;

import java.util.HashSet;
import java.util.LinkedList;
import java.util.Set;
import java.util.WeakHashMap;

import javax.media.opengl.GL3;

import de.jreality.jogl3.GlTexture;
import de.jreality.jogl3.JOGLRenderState;
import de.jreality.jogl3.JOGLSceneGraphComponentInstance.RenderableObject;
import de.jreality.jogl3.geom.GlReflectionMap;
import de.jreality.jogl3.geom.JOGLFaceSetInstance;
import de.jreality.jogl3.geom.JOGLGeometryInstance.GlUniform;
import de.jreality.jogl3.geom.JOGLGeometryInstance.GlUniformInt;
import de.jreality.jogl3.geom.JOGLGeometryInstance.GlUniformMat4;
import de.jreality.jogl3.shader.ShaderVarHash;
import de.jreality.math.Rn;
import de.jreality.shader.CommonAttributes;
import de.jreality.shader.ShaderUtility;

public class RenderableUnit {
	
	private GlTexture texture;
	private OptimizedGLShader shader;
	private boolean shaderInited = false;
	private GlReflectionMap reflMap;
	GlUniformInt combineMode;
	GlUniformMat4 texMatrix;
	
	//an Instance collection contains upto MAX_NUMBER_OBJ_IN_COLLECTION small objects.
	//This limitation is due to the maximum texture size.
	private LinkedList<InstanceCollection> instanceCollections = new LinkedList<InstanceCollection>();
	
	//contains the current Instances for all FaceSetInstances, with dead!
	private WeakHashMap<JOGLFaceSetInstance, Instance> instances = new WeakHashMap<JOGLFaceSetInstance, Instance>();
	
	//a simple set of all the new FaceSetInstances to be registered
	private WeakHashMap<JOGLFaceSetInstance, RenderableObject> registered = new WeakHashMap<JOGLFaceSetInstance, RenderableObject>();
	
	/**
	 * create a new entity containing all small geometries (below 1000 verts) with one equal texture/shader pair
	 * no local lights allowed
	 * @param t
	 */
	public RenderableUnit(GlTexture t, OptimizedGLShader s, GlReflectionMap reflMap){
		texture = t;
		shader = s;
		shaderInited = false;
		this.reflMap = reflMap;
	}
	/**
	 * register a {@link JOGLFaceSetInstance} for sending to GPU
	 * @param f
	 */
	public void register(RenderableObject o){
		registered.put((JOGLFaceSetInstance) o.geom, o);
		state = o.state;
		if(!shaderInited){
			shader.init(state.getGL());
			shaderInited = true;
		}
	}
	JOGLRenderState state;
	
	private void killAndRemove(RenderableObject o){
		Instance ins = instances.get(o);
		//we know it's alive, because instances only contains alive elements
		ins.collection.kill(ins);
		instances.remove(o);
	}
	
	/**
	 * writes all registered data (see {@link #register(JOGLFaceSetInstance)}) to the GPU
	 */
	public void update(GL3 gl){
//		System.out.println("starting to update RU");
		//check which fsi are missing in the registered ones
		//if there is an Instance in instances not belonging to a FaceSetInstance in registered, kill it
		Set<JOGLFaceSetInstance> set = instances.keySet();
		JOGLFaceSetInstance[] setA = set.toArray(new JOGLFaceSetInstance[set.size()]);
		
		for(JOGLFaceSetInstance fsi : setA){
			if(null == registered.get(fsi)){
				System.err.println("killing from RenderableUnit no " + this.toString());
				Instance ins = instances.get(fsi);
				ins.collection.kill(ins);
				instances.remove(fsi);
			}else{
				if(fsi.eap==null){
					System.err.println("killing from RenderableUnit no " + this.toString());
					Instance ins = instances.get(fsi);
					ins.collection.kill(ins);
					instances.remove(fsi);
				}else{
					boolean visible = (boolean)fsi.eap.getAttribute(ShaderUtility.nameSpace(CommonAttributes.POLYGON_SHADER, CommonAttributes.FACE_DRAW), CommonAttributes.FACE_DRAW_DEFAULT);
					if(!visible){
						System.err.println("killing from RenderableUnit no " + this.toString());
						Instance ins = instances.get(fsi);
						ins.collection.kill(ins);
						instances.remove(fsi);
					}
				}
			}
		}
		
		//order into sets newSet, lengthSet and posASet for FaceSetInstances that are completely new,
		//have changed their length or changed either positions or attributes respectively.
		HashSet<RenderableObject> newSet = new HashSet<RenderableObject>();
		HashSet<RenderableObject> lengthSet = new HashSet<RenderableObject>();
		HashSet<RenderableObject> posASet = new HashSet<RenderableObject>();
		
		for(JOGLFaceSetInstance fsi : registered.keySet()){
//			System.out.println("checking fsi " + fsi.getNode().getName() + " from registered");
			RenderableObject f = registered.get(fsi);
//			instances.get(f).upToDate = true;
			//check if new
//			System.out.println("instances contains:");
//			for(JOGLFaceSetInstance fsi2 : instances.keySet()){
//				System.out.println("Instance is " + instances.get(fsi2));
//				System.out.println("FSI is " + fsi2);
//			}
			
			if(instances.get(fsi) == null){
				//in fact it's new
				newSet.add(f);
				System.out.println("adding to new set of RenderableUnit no " + this.toString());
			}else if(f.geom.oChangedLength()){
				//is old, but changed its length
				lengthSet.add(f);
//				System.out.println("adding to oChLength set");
			}else if(f.geom.oChangedPositions()){
				//changed only positions or attributes
				posASet.add(f);
				instances.get(fsi).posUpToDate = false;
//				System.out.println("adding to oChPosA set");
			}else if(f.geom.oChangedAttributes()){
				//changed only positions or attributes
				posASet.add(f);
				instances.get(fsi).appChanged = true;
//				System.out.println("adding to oChPosA set");
			}else{
				//nothing changed, needs not be touched if not neccessary
				//do nothing here!
//				System.out.println("neither");
			}
			f.geom.resetOChangedLength();
			f.geom.resetOChangedPositionsOrAttributes();
		}
		//kill all Instances from lengthSet and move them to newSet
		for(RenderableObject f : lengthSet){
			//kill
			killAndRemove(f);
			//and add to newSet
			newSet.add(f);
		}
		//fill up InstanceCollections with all the elements from newSet
		boolean fillingUp = true;
		if(instanceCollections.size() == 0)
			fillingUp = false;
		int insCollNumber = -1;
		
		for(JOGLFaceSetInstance fsi : instances.keySet()){
//			System.out.println("fsi is " + fsi.getNode().getName());
			RenderableObject o = registered.get(fsi);
			//if fsi is invisible, it is not contained in the keySet of instances
//			if(instances.get(fsi) != null)
//			System.out.println("1updating state for " + fsi.getNode().getName());
			instances.get(fsi).state = o.state;
		}
		
		
//		System.out.println("start filling up");
//		System.out.println("InsColls.size = " + instanceCollections.size());
		while(fillingUp){
//			System.out.println("filling up...");
			insCollNumber++;
			if(insCollNumber >= instanceCollections.size())
				break;
			InstanceCollection currentCollection = instanceCollections.get(insCollNumber);
//			//defragment
//			if(currentCollection.isFragmented()){
//				//remove all current collection's dead from instances
//				currentCollection.defragment();
//			}
			//fill up with elements from newSet
			int free = currentCollection.getNumberFreeInstances();
//			System.out.println("free = " + free);
			if(free > 0){
				for(int i = 0; i < free; ){
					if(newSet.iterator().hasNext()){
//						System.out.println("newSet has next");
						RenderableObject o = newSet.iterator().next();
						if(!currentCollection.contains((JOGLFaceSetInstance) o.geom)){
							Instance ins = currentCollection.registerNewInstance((JOGLFaceSetInstance)o.geom, o.state);
							//ins could be null, because it is invisible
							if(ins != null){
//								System.out.println("2updating state for " + ins.fsi.getNode().getName());
								//ins.state = o.state;
								instances.put((JOGLFaceSetInstance) o.geom, ins);
							}
							
							i++;
//							System.out.println("here3");
						}
						newSet.remove(o);
					}else{
						fillingUp = false;
						i=free;
					}
				}
			}
//			System.out.println("updating collection");
			currentCollection.update();
		}
		
		//if newSet not empty yet, create new insColl:
		while(newSet.size() > 0){
//			System.out.println("- adding new InstanceCollection to this RU");
			
			InstanceCollection currentCollection = new InstanceCollection(shader);
			currentCollection.init(gl);
			instanceCollections.add(currentCollection);
			
			int free = currentCollection.getNumberFreeInstances();
			if(free > 0){
				for(int i = 0; i < free; i++){
					if(newSet.iterator().hasNext()){
						RenderableObject o = newSet.iterator().next();
						RenderableObject fsi = newSet.iterator().next();
						Instance ins = currentCollection.registerNewInstance((JOGLFaceSetInstance)fsi.geom, fsi.state);
						//ins could be null, because it is invisible
						if(ins != null){
//							System.out.println("3updating state for " +ins.fsi.getNode().getName());
							//ins.state = o.state;
							instances.put((JOGLFaceSetInstance) o.geom, ins);
						}
						
						newSet.remove(fsi);
					}else{
						i = free;
						fillingUp = false;
					}
				}
			}
			currentCollection.update();
		}
		
		//TODO merge the rest...
		//...not so important right now
		
		
		//update state of all instances
		//by now registered should contain the same keys as instances
		
		
		
		//clean up the registration hash map
		registered = new WeakHashMap<JOGLFaceSetInstance, RenderableObject>();
	}
	
	public void render(){
//		if(texture.getTexture2D() == null){
//			System.out.println("tex is null!!!");
//			return;
//		}
		if(instances.size() == 0)
			return;
		GL3 gl = state.getGL();
		
		float[] projection = Rn.convertDoubleToFloatArray(state.getProjectionMatrix());
		float[] inverseCamMatrix = Rn.convertDoubleToFloatArray(state.inverseCamMatrix);
		shader.useShader(gl);
		
		ShaderVarHash.bindUniform(shader, "uniforms", 1, gl);
		
		//matrices
		if(texture.getTexture2D() != null)
			ShaderVarHash.bindUniformMatrix(shader, "textureMatrix", Rn.convertDoubleToFloatArray(texture.getTexture2D().getTextureMatrix().getArray()), gl);
		ShaderVarHash.bindUniformMatrix(shader, "projection", projection, gl);
		ShaderVarHash.bindUniformMatrix(shader, "_inverseCamRotation", inverseCamMatrix, gl);
		
		//global lights in a texture
    	ShaderVarHash.bindUniform(shader, "sys_globalLights", 0, gl);
    	ShaderVarHash.bindUniform(shader, "sys_numGlobalDirLights", state.getLightHelper().getNumGlobalDirLights(), gl);
    	ShaderVarHash.bindUniform(shader, "sys_numGlobalPointLights", state.getLightHelper().getNumGlobalPointLights(), gl);
    	ShaderVarHash.bindUniform(shader, "sys_numGlobalSpotLights", state.getLightHelper().getNumGlobalSpotLights(), gl);
    	
		ShaderVarHash.bindUniform(shader, "sys_numLocalDirLights", 0, gl);
		ShaderVarHash.bindUniform(shader, "sys_numLocalPointLights", 0, gl);
		ShaderVarHash.bindUniform(shader, "sys_numLocalSpotLights", 0, gl);
		
		
		
		//bind shader uniforms not necessary
		if(texture.getTexture2D() != null)
			texture.bind(shader, gl);
		if(texture.hasTexture())
			ShaderVarHash.bindUniform(shader, "_combineMode", texture.combineMode, gl);
		
		reflMap.bind(shader, gl);
		if(reflMap.hasReflMap())
			ShaderVarHash.bindUniform(shader, "_reflectionMapAlpha", reflMap.alpha, gl);
		

    	//new way to do lights
		state.getLightHelper().bindGlobalLightTexture(gl);
    	
		for(InstanceCollection insColl : instanceCollections){
//			System.out.println("Render an insColl");
			insColl.render(gl);
		}
		
		shader.dontUseShader(gl);
	}
	
}
