package de.jreality.geometry;

import junit.framework.TestCase;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.data.Attribute;

public class TestIndexedFaceSetBug extends TestCase {

  public void testBug() {
    IndexedFaceSetFactory ifsf = new IndexedFaceSetFactory();
    ifsf.setGenerateEdgesFromFaces(true);
    ifsf.setGenerateFaceNormals(true);
    IndexedFaceSet cube = Primitives.cube();
    IndexedFaceSet icos = Primitives.icosahedron();
    
    
    double[][] verts;
    int[][] faceIndices;

    ifsf.setVertexCount(8);
    ifsf.setFaceCount(6);

    verts = cube.getVertexAttributes(Attribute.COORDINATES).toDoubleArrayArray(null);
    faceIndices = cube.getFaceAttributes(Attribute.INDICES).toIntArrayArray(null);
    ifsf.setVertexCoordinates(verts);
    ifsf.setFaceIndices(faceIndices);

    for (int i = 0; i < 30; i++) {
//      verts = cube.getVertexAttributes(Attribute.COORDINATES).toDoubleArrayArray(null);
//      faceIndices = cube.getFaceAttributes(Attribute.INDICES).toIntArrayArray(null);
      ifsf.setVertexCoordinates(verts);
//      ifsf.setFaceIndices(faceIndices);
      ifsf.update();
      System.out.println(ifsf.getIndexedFaceSet());
//      verts = icos.getVertexAttributes(Attribute.COORDINATES).toDoubleArrayArray(null);
//      faceIndices = icos.getFaceAttributes(Attribute.INDICES).toIntArrayArray(null);
//      ifsf.setFaceCount(20);
//      ifsf.setVertexCount(12);
//      ifsf.setFaceIndices(faceIndices);
//      ifsf.setVertexCoordinates(verts);
//      ifsf.update();
//      System.out.println(ifsf.getIndexedFaceSet());
    }
  }
}
