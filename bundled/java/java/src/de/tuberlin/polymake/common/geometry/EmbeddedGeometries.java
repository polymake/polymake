/* Copyright (c) 1997-2019
   Ewgenij Gawrilow, Michael Joswig, and the polymake team
   Technische Universität Berlin, Germany
   https://polymake.org

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2, or (at your option) any
   later version: http://www.gnu.org/licenses/gpl.txt.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
--------------------------------------------------------------------------------
*/

package de.tuberlin.polymake.common.geometry;

import java.util.HashMap;
import java.util.Vector;

import de.tuberlin.polymake.common.geometry.PolymakePoint;

/**
 * This class implements a set of <code>EmbeddedGeometry</code>'s all whose coordinates
 * depend on a unique embedding. Manipulating any of the geometries will manipulate all
 * geometries which are dynamically embedded.
 *
 * @author Thilo Rörig
 */
public class EmbeddedGeometries {
    
    /**
     * The underlying <code>PointSet</code> containing the coordinates on which all
     * <code>EmbeddedGeometry</code>'s of <code>geometries</code> are embedded.
     */
    protected PointSet embedding = new PointSet();
    
    /**
     * Maps the names of the <code>EmbeddedGeometry</code>'s to their index in <code>geometries</code>.
     */
    protected HashMap geomIndexMap = new HashMap(8);
    
    /**
     * An array containing <code>EmbeddedGeometry</code>'s all embedded on the same <code>embedding</code>.
     */
    protected EmbeddedGeometry[] geometries;
    
    /**
     * The name of the entire geometric structure.
     */
    protected String name = "NoName";
    
    /**
     * Creates a new <code>EmbeddedGeometries</code> instance.
     *
     * @param title title of the set of geometries
     * @param geoms the set of geometries
     */
    public EmbeddedGeometries(String title, EmbeddedGeometry[] geoms) {
        name = title;
        geometries = new EmbeddedGeometry[geoms.length];
        for(int i = 0; i < geometries.length; ++i) {
            geometries[i] = (EmbeddedGeometry)geoms[i].clone();
            geomIndexMap.put(geometries[i].getName(),new Integer(i));
        }
    }
    
    /**
     * Creates a new <code>EmbeddedGeometries</code> instance.
     *
     * @param title title of the set of geometries
     * @param emb   the underlying <code>PgPointSet</code>
     * @param geoms the set of geometries
     */
    public EmbeddedGeometries(String title, PointSet emb, EmbeddedGeometry[] geoms) {
        name = title;
        if(emb != null) {
            embedding = new PointSet(emb);
        }
        embedding.setName(title);
        geometries = new EmbeddedGeometry[geoms.length];
        for(int i = 0; i < geometries.length; ++i) {
            geometries[i] = (EmbeddedGeometry)geoms[i].clone();
            geomIndexMap.put(geometries[i].getName(),new Integer(i));
        }
    }
    
    /**
     * Move a vertex with index <code>index</code> of the <code>EmbeddedGeometry</code>
     * with name <code>geomName</code> to its new <code>coords</code> and update the embedding
     * and all other geometries on the same embedding.
     *
     * @param geomName the name of the geometry
     * @param index    index of the vertex in the geometry <code>geomName</code>
     * @param coords   the new coordinates of the vertex
     */
    public void moveVertex(String geomName,int index, double[] coords) {
        int geomIndex = ((Integer)geomIndexMap.get(geomName)).intValue();
        int vertexIndex = geometries[geomIndex].getVertexIndex(index);
    	embedding.setPointCoords(vertexIndex, coords);
        update(embedding,false);
    }
    
    /**
     * Get a vertex by <code>geomName</code> and <code>index</code>.
     *
     * @param geomName the name of the geometry
     * @param index    the index of the vertex in this geometry
     * @return         the vertex
     */
    public PolymakePoint getEmbeddedVertex(String geomName, int index) {
    	int vertexIndex = getEmbeddedVertexIndex(geomName, index);
        PolymakePoint ppt = embedding.getPoint(vertexIndex);
        return new PolymakePoint(ppt.getCoords(),Integer.toString(vertexIndex));
    }
    
    /**
     * Update the underlying embedding of all geometries.
     *
     * @param ps       the new coordinates of the embedding
     * @param clearTag if true, all vertexTags are removed
     */
    public void update(PointSet ps, boolean clearTag) {
        name = ps.getName();
        embedding.setName(name);
//        Color[] colors = embedding.getVertexColors();
        //TODO: When does this happen?
        if(embedding.getNPoints() <= ps.getNPoints()) {
            embedding.setNPoints(ps.getNPoints());
        }
        for(int i = 0; i < ps.getNPoints(); ++i) {
            PolymakePoint point = ps.getPoint(i);
            double[] coords = point.getCoords();
            if(point.getLabel() != null) {
                embedding.setPointCoords(Integer.parseInt(point.getLabel()),coords);
            } else {
                embedding.setPointCoords(i, coords);
            }
        }
//        embedding.setVertexColors(colors);
//        embedding.update(embedding);
        for(int i = 0; i < geometries.length; ++i) {
            if(geometries[i].isDynamic()) {
                geometries[i].update(embedding,clearTag);
            }
        }
    }
    
    /**
     * Get the coordinates of the vertex with index <code>index</code> of the embedding.
     *
     * @param index the index of the vertex
     * @return      the coordinates of the vertex
     */
    public double[] getVertexCoords(int index) {
        return embedding.getPoint(index).getCoords();
    }
    
    /**
     * Get the number of embedded geometries
     *
     * @return the number of embedded geometries
     */
    public int getNumberOfGeometries() {
        return geometries.length;
    }
    
    /**
     * Get the embedding.
     *
     * @return the embedding
     */
    public PointSet getEmbedding() {
        return embedding;
    }
    
    /**
     * Change the embedded geometry at index.
     *
     * @param index the index of the geometry
     * @param g     the geometry
     */
    public void setGeometry(int index, GeometryIf g) {
        geometries[index].setGeometry(g.copy());
    }
    
    /**
     * Get the embedded geometry at index.
     *
     * @param index the index of the geometry
     * @return      the geometry
     */
    public GeometryIf getGeometry(int index) {
        return geometries[index].getGeometry();
    }
    
    /**
     * Get the geometric structure of all embedded geometries.
     *
     * @return the geometric structure of all embedded geometries
     */
    public GeometryIf[] getGeometries() {
        GeometryIf[] polygonSets = new GeometryIf[geometries.length];
        for(int i = 0; i < polygonSets.length; ++i) {
            polygonSets[i] = geometries[i].getGeometry();
        }
        return polygonSets;
    }
    

    
//    /**
//     * Get the vertex colors
//     *
//     * @return the vertex colors
//     */
//    public Color[] getVertexColors() {
//        return embedding.getVertexColors();
//    }
    
    /**
     * Get the name.
     *
     * @return the name
     */
    public String getName() {
        return name;
    }
    
    /**
     * Change the name
     *
     * @param name the name
     */
    public void setName(String name) {
        this.name = name;
    }
    
    /**
     * Get a copy of the whole.
     *
     * @return a copy
     */
	public Object clone() {
        return new EmbeddedGeometries(name,embedding,geometries);
    }
	
    /**
     * Get a String representation of the embedding and all the embedded geometries.
     *
     * @return String representation of the instance
     */
    public String toString() {
        String msg = new String();
        msg += "name = " + name + "\n";
        msg += "embedding = " + embedding.toString() + "\n";
        for(int i = 0; i < geometries.length; ++i) {
            msg += "geometries["+i+"]=" + geometries[i].toString();
        }
        return msg;
    }
    
    /**
     * Get the marked vertices
     *
     * @return the marked vertices
     */
	public Vector getMarkedVertices()  {
        Vector markedVertices = new Vector();
        for(int j=0; j < embedding.getNPoints(); ++j) {
            embedding.setMarkedPoint(j,false);
        }
        for(int j = 0; j < geometries.length; ++j) {
            GeometryIf geom = getGeometry(j);
            for (int i = 0; i < geom.getNumVertices(); i++) {
                if (geom.getMarked(i)) {
                    markedVertices.add(Integer.toString(geometries[j].vertexList[i]));
                    double[] vertex = geom.getVertexCoords(i);
                    embedding.setPointCoords(geometries[j].vertexList[i],vertex);
                    embedding.setMarkedPoint(geometries[j].vertexList[i],true);
                }
            }
        }
        return markedVertices;
	}

	public int getEmbeddedVertexIndex(String geomName, int index) {
		int geomIndex = ((Integer)geomIndexMap.get(geomName)).intValue();
        return geometries[geomIndex].getVertexIndex(index);
	}

}
