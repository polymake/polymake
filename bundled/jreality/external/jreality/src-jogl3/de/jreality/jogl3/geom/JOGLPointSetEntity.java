package de.jreality.jogl3.geom;

import java.util.HashMap;
import java.util.Set;

import javax.media.opengl.GL3;

import de.jreality.jogl3.shader.GLVBO;
import de.jreality.jogl3.shader.GLVBOFloat;
import de.jreality.jogl3.shader.GLVBOInt;
import de.jreality.math.Rn;
import de.jreality.scene.PointSet;
import de.jreality.scene.data.Attribute;
import de.jreality.scene.data.DataList;
import de.jreality.scene.data.DoubleArray;
import de.jreality.scene.data.DoubleArrayArray;
import de.jreality.scene.data.IntArray;
import de.jreality.scene.data.StringArray;
import de.jreality.scene.event.GeometryEvent;

public class JOGLPointSetEntity extends JOGLGeometryEntity {

	private HashMap<String, GLVBO> pointVbos = new HashMap<String, GLVBO>();
	public Label[] labels = new Label[0];
	public int labelsChangedNo = 0;
	
	public GLVBO getPointVBO(String s) {
		// TODO Auto-generated method stub
		return pointVbos.get(s);
	}
	public int getNumPointVBOs(){
		return pointVbos.size();
	}
	public GLVBO[] getAllPointVBOs(){
		GLVBO[] ret = new GLVBO[pointVbos.size()];
		Set<String> keys = pointVbos.keySet();
		int i = 0;
		for(String s : keys){
			ret[i] = pointVbos.get(s);
			i++;
		}
		return ret;
	}
	
	//protected GLVBOFloat vertexVBO = null;
	

	public JOGLPointSetEntity(PointSet node) {
		super(node);
	}

	public void geometryChanged(GeometryEvent ev) {
//		System.out.println("JOGLPointSetEntity.geometryChanged()");
		dataUpToDate = false;
	}

	public boolean updateData(GL3 gl) {
		//if (!dataUpToDate) {
			pointVbos.clear();
			//TODO convert to automatic creation of vbos
			PointSet ps = (PointSet) getNode();
			
			Set<Attribute> aS = ps.getVertexAttributes().storedAttributes();
			for(Attribute a : aS){
				String shaderName = "";
				for(String s : a.getName().split(" ")){
					shaderName = shaderName + s;
				}
				//System.out.println(ps.getName() + "point set vertex attribute: "+a.getName());
				DataList attribs = ps.getVertexAttributes(a);
				if(isDoubleArray(attribs.getStorageModel())){
					double[] inflatedAttributeArray = new double[ps.getNumPoints()];
					DoubleArray dA = (DoubleArray)attribs;
					for(int i = 0; i < ps.getNumPoints(); i++){
						inflatedAttributeArray[i] = dA.getValueAt(i);
					}
					pointVbos.put("vertex_"+shaderName, new GLVBOFloat(gl, Rn.convertDoubleToFloatArray(inflatedAttributeArray), "vertex_"+a.getName(), 1));
				}else if(isDoubleArrayArray(attribs.getStorageModel())){
					
					//the array containing one item per index
					double[] inflatedAttributeArray = new double[ps.getNumPoints()*4];
					//count = 0;
					//for each index in the indexArray
					for(int i = 0; i < ps.getNumPoints(); i++){
						//we retrieve the vertex attribute
						//int j = indexArray[i];
						DoubleArray dA = (DoubleArray)attribs.get(i);
						
						inflatedAttributeArray[4*i+0] = dA.getValueAt(0);
						if(dA.size() > 1)
							inflatedAttributeArray[4*i+1] = dA.getValueAt(1);
						else
							inflatedAttributeArray[4*i+1] = 0;
						if(dA.size() > 2)
							inflatedAttributeArray[4*i+2] = dA.getValueAt(2);
						else
							inflatedAttributeArray[4*i+2] = 0;
						if(dA.size() > 3)
							inflatedAttributeArray[4*i+3] = dA.getValueAt(3);
						else
							inflatedAttributeArray[4*i+3] = 1;
					}
					pointVbos.put("vertex_"+shaderName, new GLVBOFloat(gl, Rn.convertDoubleToFloatArray(inflatedAttributeArray), "vertex_"+a.getName()));
					
					//System.out.println("creating in PointSetEntity " + "vertex_"+shaderName);
				
				}else if(isIntArray(attribs.getStorageModel())){
					//the array containing one item per index
					int[] inflatedAttributeArray = new int[ps.getNumPoints()*4];
					//count = 0;
					//for each index in the indexArray
					for(int i = 0; i < ps.getNumPoints(); i++){
						//we retrieve the vertex attribute
						IntArray dA = (IntArray)attribs.get(i);
						
						inflatedAttributeArray[4*i+0] = dA.getValueAt(0);
						if(dA.size() > 1)
							inflatedAttributeArray[4*i+1] = dA.getValueAt(1);
						else
							inflatedAttributeArray[4*i+1] = 0;
						if(dA.size() > 2)
							inflatedAttributeArray[4*i+2] = dA.getValueAt(2);
						else
							inflatedAttributeArray[4*i+2] = 0;
						if(dA.size() > 3)
							inflatedAttributeArray[4*i+3] = dA.getValueAt(3);
						else
							inflatedAttributeArray[4*i+3] = 1;
					}
					pointVbos.put("vertex_"+shaderName, new GLVBOInt(gl, inflatedAttributeArray, "vertex_"+a.getName()));
//					System.out.println("creating " + "vertex_"+a.getName());
				
				}else if(/*a.getName().equals("labels") && */isStringArray(attribs.getStorageModel())){
					labelsChangedNo++;
					StringArray SA = (StringArray)attribs;
					int count = 0;
					for(int i = 0; i < ps.getNumPoints(); i++){
						String s = SA.getValueAt(i);
						if(!s.equals(""))
							count++;
					}
					labels = new Label[count];
					
					//coordinates of the 8 vertices
					DataList attrib = ps.getVertexAttributes(Attribute.COORDINATES);
					count = 0;
					for(int i = 0; i < ps.getNumPoints(); i++){
						String s = SA.getValueAt(i);
						if(!s.equals("")){
							labels[count] = new Label();
							labels[count].text = s;
							
							
							double[] du = new double[4];
							DoubleArrayArray dA = (DoubleArrayArray)attrib;
							du[0] = dA.getValueAt(count).getValueAt(0);
							du[1] = dA.getValueAt(count).getValueAt(1);
							du[2] = dA.getValueAt(count).getValueAt(2);
							du[3] = 1;
							
							labels[count].position = du;
							count++;
						}
					}
					
//					System.out.println("creating " + "point_"+a.getName());
				}else{
					System.out.println("PSE1: not knowing what to do with " + attribs.getStorageModel().toString()+" "+attribs.getStorageModel().getClass().toString() + a.getName());
				}
				//System.out.println("face attribute names: " + a.getName());
			}
			
			//dataUpToDate = true;
			
//			DataList vA = ps.getVertexAttributes(Attribute.COORDINATES);
//			System.out.println("num points = "+ ps.getNumPoints());
//			double[] vertdata = new double[ps.getNumPoints()*3];
//			vA.toDoubleArray(vertdata);
//			
//			vertexVBO = new GLVBOFloat(gl, Rn.convertDoubleToFloatArray(vertdata), "vertex.coordinates");
//			
//			
//			System.out.println("data up to date: "+getNode().getName());
			
		//}
			return false;
	}
}
