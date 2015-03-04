package de.jreality.reader.obj;

import java.util.ArrayList;
import java.util.Comparator;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.Set;
import java.util.TreeSet;
import java.util.logging.Logger;

import de.jreality.geometry.IndexedFaceSetUtility;
import de.jreality.scene.Appearance;
import de.jreality.scene.Geometry;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.data.Attribute;
import de.jreality.scene.data.StorageModel;

public class OBJModel {
	
	private static Logger logger = Logger.getLogger(OBJModel.class.getSimpleName());

	private HashMap<String, OBJGroup> groups = new HashMap<String, OBJGroup>();
	private List<OBJGroup> activeGroups = new LinkedList<OBJGroup>();
	private OBJGroup defaultGroup = new OBJGroup("default");
	private HashMap<String, Appearance> materials = new HashMap<String, Appearance>();

	private List<double[]> vertexCoords = new ArrayList<double[]>();
	private List<double[]> textureCoords = new ArrayList<double[]>();
	private List<double[]> normalCoords = new ArrayList<double[]>();
	
	public OBJModel() {
		groups.put(defaultGroup.getName(), defaultGroup);
		activeGroups.add(defaultGroup);
	}
		
	public void addVertexCoords(double[] coords) {
		vertexCoords.add(coords);
	}

	public void addNormalCoords(double[] n) {
		normalCoords.add(n);
	}

	public void addTextureCoords(double[] tex) {
		textureCoords.add(tex);
	}

	public void setActiveGroups(List<String> groupNames) {
		activeGroups.clear();
		if(groupNames.size() == 0) {
			logger.fine("Empty group statement. Setting default group active.");
			activeGroups.add(defaultGroup);
		} else {
			for(String name : groupNames) {
				OBJGroup g = groups.get(name);
				if(g == null) {
					g = new OBJGroup(name);
					groups.put(name,g);
				}
				activeGroups.add(g);
			}
		}
	}

	public void addPoints(List<OBJVertex> points) {
		for(OBJGroup g : activeGroups) {
			g.addAllPoints(points);
		}
	}

	public void addLine(List<OBJVertex> l) {
		for(OBJGroup g : activeGroups) {
			g.addLine(l);
		}
	}

	public void addFace(List<OBJVertex> face) {
		for(OBJGroup g : activeGroups) {
			g.addFace(face);
		}
	}

	public void addMaterial(Appearance a) {
		materials.put(a.getName(), a);
	}

	public void useMaterial(String mtlName) {
		Appearance mtl = materials.get(mtlName);
		if (mtl == null) {
			logger.warning("Warning: Unknown material with name [" + mtlName + "].");
		} else {
			for(OBJGroup g: activeGroups) {
				g.setMaterial(mtl);
			}
		}
	}

	public List<SceneGraphComponent> getComponents(boolean useMultipleTexAndNormalCoords, boolean generateEdgesFromFaces) {
		List<SceneGraphComponent> cps = new LinkedList<SceneGraphComponent>();
		for (OBJGroup g : groups.values()) {
			if (g.hasGeometry()) {
				cps.add(createComponent(g,useMultipleTexAndNormalCoords,generateEdgesFromFaces));
			} else {
				logger.fine("Ignoring group " + g.getName() + " [has no geometry]");
			}
		}
		return cps;
	}
	
	private SceneGraphComponent createComponent(OBJGroup g, boolean useMultipleTexAndNormalCoords, boolean generateEdgesFromFaces) {
		SceneGraphComponent ret = new SceneGraphComponent();
		ret.setName(g.getName());
		ret.setAppearance(g.getMaterial());
		ret.setGeometry(createGeometry(g, useMultipleTexAndNormalCoords,generateEdgesFromFaces));
		return ret;
	}
	
	private Geometry createGeometry(OBJGroup g, boolean useMultipleTexAndNormalCoords, boolean generateEdgesFromFaces) {
		
		List<List<OBJVertex>> lines = g.getLines();
		List<OBJVertex> points = g.getPoints();
		List<List<OBJVertex>> faces = g.getFaces();
		
		OBJIndexFactory vd = new OBJIndexFactory(points, lines, faces, useMultipleTexAndNormalCoords);
		
		IndexedFaceSet ifs = new IndexedFaceSet();

		//Vertices
		ArrayList<double[]> vertices = extractCoords(vertexCoords, vd.extractVertexIndices()); 
		ifs.setVertexCountAndAttributes(Attribute.COORDINATES, StorageModel.DOUBLE3_ARRAY.createReadOnly(vertices.toArray(new double[vertices.size()][])));
		
		//Faces
		ArrayList<int[]> faceIndices = vd.extractIndicesList(faces); 
		ifs.setFaceCountAndAttributes(Attribute.INDICES, StorageModel.INT_ARRAY_ARRAY.createReadOnly(faceIndices.toArray(new int[faceIndices.size()][])));
		
		// check if texture coordinates are available and if size fits
		if(textureCoords.size() != 0) {
			ArrayList<double[]> vertexTex = extractCoords(textureCoords,vd.extractTextureIndices());
			if (vertexTex != null) {
				double[][] vTexArray = new double[vertexTex.size()][];
				vertexTex.toArray(vTexArray);
				int numPerEntry = 2;
				if (vTexArray.length != 0) {
					numPerEntry = vTexArray[0].length;
				}
				ifs.setVertexAttributes(Attribute.TEXTURE_COORDINATES, StorageModel.DOUBLE_ARRAY.array(numPerEntry).createReadOnly(vTexArray));
			}
		}
		
		if(normalCoords.size() != 0) {
			ArrayList<double[]> vertexNorms = extractCoords(normalCoords, vd.extractNormalIndices());
			if (!vertexNorms.isEmpty()) {
					ifs.setVertexAttributes(Attribute.NORMALS, StorageModel.DOUBLE3_ARRAY.createReadOnly(vertexNorms.toArray(new double[vertexNorms.size()][])));
			}
		}	

		boolean hasVertexNormals = ifs.getVertexAttributes(Attribute.NORMALS) != null;
		
		if (!hasVertexNormals) {
			IndexedFaceSetUtility.calculateAndSetVertexNormals(ifs);
		}
		
		if (!hasVertexNormals) {
			IndexedFaceSetUtility.calculateAndSetFaceNormals(ifs);
		}
		
		//Lines
		Set<int[]> lineIndices = new TreeSet<int[]>(new EdgeComparator());
		if(lines.size() > 0) {
			lineIndices.addAll(vd.extractIndicesList(lines)); 
		}
		if(faces.size() > 0 && generateEdgesFromFaces) {
			if(lines.size() == 0) {
				IndexedFaceSetUtility.calculateAndSetEdgesFromFaces(ifs);
			} else {
				int[][] edges = IndexedFaceSetUtility.edgesFromFaces(faceIndices.toArray(new int[faceIndices.size()][])).toIntArrayArray(null);
				for(int i = 0; i < edges.length; ++i) {
					lineIndices.add(edges[i]);
				}
			}
		}
		
		if (lineIndices.size() != 0) {
			ifs.setEdgeCountAndAttributes(Attribute.INDICES,StorageModel.INT_ARRAY_ARRAY.createReadOnly(lineIndices.toArray(new int[lineIndices.size()][])));
		}
		return ifs;
	}
	
	private ArrayList<double[]> extractCoords(List<double[]> coords, List<Integer> indices) {
		ArrayList<double[]> list = new ArrayList<double[]>();
		for (Integer i : indices) {
			if(i == 0) {
				list.add(new double[]{0,0,0});
			} else if(i > 0) {
				list.add(coords.get(i-1));
			} else {
				list.add(coords.get(coords.size()+i));
			}
		}
		return list;
	}
	
	private class EdgeComparator implements Comparator<int[]> {

		@Override
		public int compare(int[] o1, int[] o2) {
			if(o1.length != o2.length || o1.length < 2 || o2.length < 2) {
				return 1;
			} else {
				int[] tmp1 = (o1[0] > o1[1])?new int[]{o1[1],o1[0]}:o1;
				int[] tmp2 = (o2[0] > o2[1])?new int[]{o2[1],o2[0]}:o2;
				if(tmp1[0] - tmp2[0] != 0) {
					return tmp1[0] - tmp2[0];
				} else if(tmp1[1] - tmp2[1] != 0) {
					return tmp1[1] - tmp2[1];
				} else {
					return 0;
				}
			}
		}
		
		
		
	}
}
