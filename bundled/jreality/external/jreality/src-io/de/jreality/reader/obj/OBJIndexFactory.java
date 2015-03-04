package de.jreality.reader.obj;

import java.util.ArrayList;
import java.util.Comparator;
import java.util.LinkedHashMap;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.TreeMap;
import java.util.logging.Logger;


public class OBJIndexFactory {
	
	private Logger logger = Logger.getLogger(OBJIndexFactory.class.getSimpleName());
	
	/**
	 * creates indices for triples of vertex/tex/normal
	 */
	private TreeMap<OBJVertex, Integer> 
		vertexIndexMap = null;

	private ArrayList<OBJVertex> 
		list = new ArrayList<OBJVertex>();
	
	private Map<Integer, LinkedHashSet<OBJVertex>> 
		vertexClusters = new LinkedHashMap<Integer, LinkedHashSet<OBJVertex>>();
	
	private boolean
		useMultipleTexAndNormalCoords = true;

	public OBJIndexFactory(List<OBJVertex> points, List<List<OBJVertex>> lines, List<List<OBJVertex>> faces, boolean useMultipleTexAndNormalCoords) {

		this.useMultipleTexAndNormalCoords = useMultipleTexAndNormalCoords;
		vertexIndexMap = new TreeMap<OBJVertex, Integer>(new VertexComparator(useMultipleTexAndNormalCoords));

		if(points != null) {
			for(OBJVertex v : points) {
				addVertex(v);
			}
		}

		if(lines != null) {
			for(List<OBJVertex> line : lines) {
				for(OBJVertex v : line) {
					addVertex(v);
				}
			}
		}

		if(faces != null) {
			for(List<OBJVertex> face : faces) {
				for(OBJVertex v : face) {
					addVertex(v);
				}
			}
		}
	}

	public Integer getId(OBJVertex v) {
		Integer index = vertexIndexMap.get(v); 
		if(useMultipleTexAndNormalCoords) {
			if(index == null) {
				Set<OBJVertex> mergeableVertices = getMergeableVertices(v);
				if(mergeableVertices.size() != 0) {
					OBJVertex knownVertex = mergeableVertices.iterator().next();
					int mergeableIndex = vertexIndexMap.get(knownVertex);
					return mergeableIndex;
				} 
				logger.severe("Could not find vertex index for vertex " + v.toString());
				return null;
			} else {
				return index;
			}
		} else {
			if(index != null) {
				return index;
			} else {
				return vertexIndexMap.get(new OBJVertex(v.getVertexIndex(), 0,0));
			}
		}
	}

	private Set<OBJVertex> getMergeableVertices(OBJVertex v) {
		Set<OBJVertex> verts = new LinkedHashSet<OBJVertex>();
		for(OBJVertex k : vertexClusters.get(v.getVertexIndex())) {
			if(mergeableVertices(k, v)) {
				verts.add(k);
			}
		}
		return verts;
	}

	public List<Integer> extractVertexIndices() {
		List<Integer> vertexIndices = new ArrayList<Integer>();
		for(OBJVertex v : list) {
			vertexIndices.add(v.getVertexIndex());
		}
		return vertexIndices;
	}

	public List<Integer> extractTextureIndices() {
		List<Integer> textureIndices = new ArrayList<Integer>();
		for(OBJVertex v : list) {
			textureIndices.add(v.getTextureIndex());
		}
		return textureIndices;		
	}

	public List<Integer> extractNormalIndices() {
		List<Integer> normalIndices = new ArrayList<Integer>();
		for(OBJVertex v : list) {
			normalIndices.add(v.getNormalIndex());
		}
		return normalIndices;		
	}

	private int addVertex(OBJVertex v) {
		if(vertexClusters.get(v.getVertexIndex()) == null) {
			vertexClusters.put(v.getVertexIndex(),new LinkedHashSet<OBJVertex>());
		}
		if(useMultipleTexAndNormalCoords) {
			Integer index = vertexIndexMap.get(v);
			if(index == null) {
				Set<OBJVertex> mergeableVerts = getMergeableVertices(v);
				if(mergeableVerts.size() != 0) {
					OBJVertex knownVertex = mergeableVerts.iterator().next();
					vertexClusters.get(knownVertex.getVertexIndex()).remove(knownVertex);
					int sameVertexIndex = vertexIndexMap.get(knownVertex);
					mergeVertices(knownVertex, v);
					vertexClusters.get(knownVertex.getVertexIndex()).add(knownVertex);
					vertexIndexMap.put(knownVertex, sameVertexIndex);
					list.set(sameVertexIndex, knownVertex);
					return sameVertexIndex;
				} else {
					vertexClusters.get(v.getVertexIndex()).add(v);
					return createID(v);
				}
			} else {
				return index;
			}
		} else {
			Integer sameVertexIndex = vertexIndexMap.get(new OBJVertex(v.getVertexIndex(), 0, 0));
			if(sameVertexIndex != null) {
				OBJVertex knownVertex = list.get(sameVertexIndex);
				if(mergeableVertices(v, knownVertex)) {
					mergeVertices(knownVertex, v);
					vertexIndexMap.put(knownVertex, sameVertexIndex);
					list.set(sameVertexIndex, knownVertex);
				} else { 
					logger.warning("Discarding normal and texture of vertex "+v+", since vertex "+knownVertex+" is already known.");
				}
				return sameVertexIndex;
			} else {
				return createID(v);
			}
		}
	}

	private int createID(OBJVertex v) {
		int vIndex = list.size();
		vertexIndexMap.put(v, vIndex);
		list.add(v);
		return vIndex;
	}

	private class VertexComparator implements Comparator<OBJVertex> {

		private boolean
		strict = false;

		public VertexComparator(boolean strict) {
			this.strict = strict;
		}

		@Override
		public int compare(OBJVertex o1, OBJVertex o2) {
			if(strict) {
				return strongCompare(o1, o2);
			} else {
				return weakCompare(o1, o2);
			}
		}

		private int strongCompare(OBJVertex o1, OBJVertex o2) {
			int compareV = o1.getVertexIndex() - o2.getVertexIndex();
			int compareT = o1.getTextureIndex() - o2.getTextureIndex();
			int compareN = o1.getNormalIndex() - o2.getNormalIndex();
			if(compareV != 0) {
				return compareV;
			} else if(compareT != 0) {
				return compareT;
			} else if(compareN != 0) {
				return compareN;
			} else {
				return 0;
			}
		}

		private int weakCompare(OBJVertex o1, OBJVertex o2) {
			int compareV = compareIndex(o1.getVertexIndex(),o2.getVertexIndex());
			int compareT = compareIndex(o1.getTextureIndex(),o2.getTextureIndex());
			int compareN = compareIndex(o1.getNormalIndex(),o2.getNormalIndex());
			if(compareV != 0) {
				return compareV;
			} else if(compareT != 0) {
				return compareT;
			} else if(compareN != 0) {
				return compareN;
			} else {
				return 0;
			}
		}
	}

	static int compareIndex(int o1, int o2) {
		if(o1 == 0 || o2 == 0) {
			return 0;
		} else {
			return o1-o2;
		}
	}

	boolean mergeVertices(OBJVertex v1, OBJVertex v2) {

		if( !mergeableVertices(v1, v2) ) {
			return false;
		}
		if(v1.getTextureIndex() == 0) {
			v1.setTextureIndex(v2.getTextureIndex());
		}
		if(v1.getNormalIndex() == 0) {
			v1.setNormalIndex(v2.getNormalIndex());
		}
		return true;
	}

	private boolean mergeableVertices(OBJVertex v1, OBJVertex v2) {
		return v1.getVertexIndex() == v2.getVertexIndex() &&
				compareIndex(v1.getTextureIndex(),v2.getTextureIndex()) == 0 &&
				compareIndex(v1.getNormalIndex(),v2.getNormalIndex()) == 0;
	}

	void reset() {
		vertexIndexMap.clear();
	}

	int size() {
		return vertexIndexMap.size();
	}

	int vertexId(int id) {
		return list.get(id).getVertexIndex();
	}

	int texId(int id) {
		return list.get(id).getTextureIndex();
	}

	int normalId(int id) {
		return list.get(id).getNormalIndex();
	}

	public ArrayList<int[]> extractIndicesList(List<List<OBJVertex>> faces) {
		ArrayList<int[]> faceIndices = new ArrayList<int[]>();
		for(List<OBJVertex> face : faces) {
			faceIndices.add(extractIndices(face));
		}
		return faceIndices;
	}

	private int[] extractIndices(List<OBJVertex> face) {
		int[] indices = new int[face.size()];
		int i = 0;
		for(OBJVertex v : face) {
			indices[i++] = getId(v);
		}
		return indices;
	}
}
