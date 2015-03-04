package de.jreality.reader.obj;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Iterator;
import java.util.List;
import java.util.Set;
import java.util.logging.Logger;

import de.jreality.reader.ParserMTL;
import de.jreality.scene.Appearance;

class OBJGroup {

	private Logger logger = Logger.getLogger(OBJGroup.class.getSimpleName());
	
	private String 
		name = "NN";
	private Appearance 
		material = ParserMTL.createDefault();
	private List<OBJVertex> 
		points = new ArrayList<OBJVertex>();
	private List<List<OBJVertex>> 
		lines = new ArrayList<List<OBJVertex>>();
	private List<List<OBJVertex>> 
		faces = new ArrayList<List<OBJVertex>>();

	public OBJGroup(String name) {
		this.name = name;
	}

	public void addAllPoints(List<OBJVertex> pts) {
		for(OBJVertex v : pts) {
			points.add(v);
		}
	}
	
	public void addLine(List<OBJVertex> line) {
		lines.add(line);
	}

	public void addFace(List<OBJVertex> face) {
		faces.add(face);
	}

	public String getName() {
		return name;
	}

	public boolean hasGeometry() {
		return faces.size() > 0 || lines.size() > 0 || points.size() > 0;
	}

	void setMaterial(Appearance a) {
		if (a == null) {
			logger.warning("Trying to set material appearance to null");
			return;
		}
		Set<String> lst = a.getStoredAttributes();
		for (Iterator<String> i = lst.iterator(); i.hasNext();) {
			String aName = (String) i.next();
			material.setAttribute(aName, a.getAttribute(aName));
		}
	}

	public Appearance getMaterial() {
		return material;
	}

	public List<List<OBJVertex>> getLines() {
		return Collections.unmodifiableList(lines);
	}

	public List<OBJVertex> getPoints() {
		return Collections.unmodifiableList(points);
	}

	public List<List<OBJVertex>> getFaces() {
		return Collections.unmodifiableList(faces);
	}

}
