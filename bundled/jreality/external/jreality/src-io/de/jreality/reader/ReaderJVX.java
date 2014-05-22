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


package de.jreality.reader;

import java.io.IOException;
import java.net.URL;
import java.util.LinkedList;
import java.util.Stack;

import javax.xml.parsers.ParserConfigurationException;
import javax.xml.parsers.SAXParser;
import javax.xml.parsers.SAXParserFactory;

import org.xml.sax.Attributes;
import org.xml.sax.EntityResolver;
import org.xml.sax.InputSource;
import org.xml.sax.SAXException;
import org.xml.sax.XMLReader;
import org.xml.sax.helpers.DefaultHandler;

import de.jreality.geometry.IndexedFaceSetUtility;
import de.jreality.scene.Appearance;
import de.jreality.scene.Geometry;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.IndexedLineSet;
import de.jreality.scene.PointSet;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SceneGraphVisitor;
import de.jreality.scene.data.Attribute;
import de.jreality.scene.data.DataListSet;
import de.jreality.scene.data.StorageModel;
import de.jreality.shader.CommonAttributes;
import de.jreality.util.Input;
import de.jreality.util.LoggingSystem;


/**
 *
 * Simple reader for JVX files (JavaView format).
 *
 * @author timh, Steffen Weissmann
 *
 */
public class ReaderJVX extends AbstractReader {
	
  public void setInput(Input input) throws IOException {
    super.setInput(input);
    SAXParserFactory parserFactory = SAXParserFactory.newInstance();
    parserFactory.setValidating(false);
    try {
        SAXParser parser = parserFactory.newSAXParser();
        XMLReader reader= parser.getXMLReader(); 
        reader.setEntityResolver(new Resolver());
        Handler handler = new Handler();
		reader.setContentHandler(handler);
		InputSource src=new InputSource(input.getInputStream());
		reader.parse(src); 
        root = handler.getRoot();
    } catch (ParserConfigurationException e) {
        IOException ie = new IOException(e.getMessage());
        ie.initCause(e);
        throw ie;
    } catch (SAXException e) {
      IOException ie = new IOException(e.getMessage());
      ie.initCause(e);
      throw ie;
    }
  }
  
  static class Resolver implements EntityResolver {
	    public InputSource resolveEntity(String publicId, String systemId) throws SAXException, IOException {
	    	Input dtd = null;
	    	try {
	    		dtd = Input.getInput("jvx.dtd");
	    	} catch (Exception e) {
	    		// not found
	    	}
//	    	if (dtd == null) try {
//	    		dtd = Input.getInput(ReaderJVX.class.getResource("jvx.dtd"));
//	    		System.out.println("found via class");
//	    	} catch (Exception e) {
//	    		// not found
//	    	}
	    	if (dtd == null) try {
	    		dtd = Input.getInput(new URL(systemId));
	    	} catch (Exception e) {
	    		// not found
	    	}
	    	if (dtd != null) try {
	    		return new InputSource(dtd.getInputStream());
	    	} catch (Exception e) {
	    		// jvx.dtd not available...
	    	}
    		throw new RuntimeException("could not find jvx.dtd - download http://www.javaview.de/rsrc/jvx.dtd into the execution dir");
	    }
  }
  
  static class Handler extends DefaultHandler {
		private enum CharData {
			NONE, POINT, LINE, FACE, COLOR, NORMAL;
		}

	SceneGraphComponent root = new SceneGraphComponent();
    Stack<SceneGraphComponent> componentStack = new Stack<SceneGraphComponent>();
    SceneGraphComponent currentComponent;

    DataListSet vertexAttributes;
    DataListSet edgeAttributes;
    DataListSet faceAttributes;
    
    private CharData currentCharData=CharData.NONE;

    LinkedList<double[]> currentPoints;
    LinkedList<double[]> currentNormals;
    LinkedList<double[]> currentColors;

    LinkedList<int[]> currentEdges;
    LinkedList<int[]> currentFaces;

    LinkedList<String> currentLabels;
    
    public SceneGraphComponent getRoot() {
        return root;
    }
    
    @Override
    public void startDocument() throws SAXException {
        currentComponent = root;
        super.startDocument();
    }
    
    @Override
    public void endDocument() throws SAXException {
        if(componentStack.size()!= 0) throw new RuntimeException(" nonempty stack at end of jvx file.");
        super.endDocument();
    }
    
    @Override
    public void startElement(String uri, String localName, String qName,
            Attributes attributes) throws SAXException {
        LoggingSystem.getLogger(this).fine("start elem: qName "+qName);

        if(qName.equals("geometry")) {
            SceneGraphComponent c = new SceneGraphComponent();
            Appearance ap = new Appearance();
            c.setAppearance(ap);
            if(currentComponent!= null) currentComponent.addChild(c);
            componentStack.push(currentComponent);
            currentComponent = c;
            return;
        }

        if(qName.equals("pointSet")) {
        	String point = attributes.getValue("point");
            if(point!= null && point.equals("show")) 
                currentComponent.getAppearance().setAttribute(CommonAttributes.VERTEX_DRAW, true);
            else
                currentComponent.getAppearance().setAttribute(CommonAttributes.VERTEX_DRAW, false);
            return;
        }

        if(qName.equals("points")) {
        	currentPoints = new LinkedList<double[]>();
        	currentLabels = new LinkedList<String>();
            return;
        }

        if(qName.equals("p")) {
        	if (currentPoints == null) return;
        	currentLabels.add(attributes.getValue("name"));
        	currentCharData = CharData.POINT;
            return;
        }

        if(qName.equals("lineSet")) {
            String point = attributes.getValue("line");
            if(point!= null && point.equals("show")) 
                currentComponent.getAppearance().setAttribute(CommonAttributes.EDGE_DRAW, true);
            else 
                currentComponent.getAppearance().setAttribute(CommonAttributes.EDGE_DRAW, false);
            return;
        }
 
       if(qName.equals("lines")) {
    	   currentEdges = new LinkedList<int[]>();
    	   currentLabels = new LinkedList<String>();
           return;
        }

        if(qName.equals("l")) {
        	if (currentEdges == null) return;
        	currentLabels.add(attributes.getValue("name"));
        	currentCharData = CharData.LINE;
        	return;
        }

        if(qName.equals("faceSet")) {
        	String point = attributes.getValue("face");
            if(point!= null && point.equals("show")) 
                currentComponent.getAppearance().setAttribute(CommonAttributes.FACE_DRAW, true);
            else 
                currentComponent.getAppearance().setAttribute(CommonAttributes.FACE_DRAW, false);
            return;
        }

        if(qName.equals("faces")) {
        	currentFaces = new LinkedList<int[]>();
        	currentLabels = new LinkedList<String>();
        	return;
        }

        if(qName.equals("f")) {
        	if (currentFaces == null) return;
        	currentLabels.add(attributes.getValue("name"));
        	currentCharData = CharData.FACE;
        	return;
        }

        if(qName.equals("colors")) {
        	currentColors = new LinkedList<double[]>();
            return;
        }

        if(qName.equals("c")) {
        	if (currentColors == null) return;
        	currentCharData = CharData.COLOR;
            return;
        }

        if(qName.equals("normals")) {
        	currentNormals = new LinkedList<double[]>();
            return;
        }

        if(qName.equals("n")) {
        	if (currentNormals == null) return;
        	currentCharData = CharData.NORMAL;
            return;
        }
        
        // unhandled elements
        currentCharData = CharData.NONE;
        // if (!"nb".equals(qName)) System.out.println("unhandled element: "+qName);
    }

    @Override
    public void endElement(String uri, String localName, String qName)
            throws SAXException {
    	charactersImpl();
        currentCharData = CharData.NONE;
        LoggingSystem.getLogger(this).fine("end elem: qName "+qName);
        if(qName.equals("geometry")) {
        	Geometry geom = faceAttributes != null ? new IndexedFaceSet() : edgeAttributes != null ? new IndexedLineSet() : new PointSet();
            currentComponent.setGeometry(geom);           
            currentComponent.childrenWriteAccept(new SceneGraphVisitor() {
            	@Override
            	public void visit(IndexedFaceSet i) {
            		visit((IndexedLineSet)i);
            		i.setFaceCountAndAttributes(faceAttributes);
            		if (!faceAttributes.containsAttribute(Attribute.NORMALS)
            			&& (edgeAttributes == null || !edgeAttributes.containsAttribute(Attribute.NORMALS))) {
            			IndexedFaceSetUtility.calculateAndSetFaceNormals(i);
            		}
            	}
            	@Override
            	public void visit(IndexedLineSet g) {
            		visit((PointSet)g);
            		if (edgeAttributes != null) g.setEdgeCountAndAttributes(edgeAttributes);
            	}
            	@Override
            	public void visit(PointSet p) {
            		p.setVertexCountAndAttributes(vertexAttributes);
            	}
            }, false, false, false, false, true, false);
            vertexAttributes = edgeAttributes = faceAttributes = null;
            currentComponent = (SceneGraphComponent) componentStack.pop();
        }
        
        if(qName.equals("pointSet")) {
    		double[][] points = currentPoints.toArray(new double[0][]);
        	vertexAttributes = new DataListSet(points.length);
    		vertexAttributes.addReadOnly(Attribute.COORDINATES, StorageModel.DOUBLE_ARRAY_ARRAY, points);
    		if (currentNormals != null && !currentNormals.isEmpty()) {
    			double[][] normals = currentNormals.toArray(new double[0][]);
    			vertexAttributes.addReadOnly(Attribute.NORMALS, StorageModel.DOUBLE_ARRAY_ARRAY, normals);
    		}
    		if (currentColors != null && !currentColors.isEmpty()) {
    			double[][] colors = currentColors.toArray(new double[0][]);
    			vertexAttributes.addReadOnly(Attribute.COLORS, StorageModel.DOUBLE_ARRAY_ARRAY, colors);
    		}
    		if (!currentLabels.isEmpty()) {
    			String[] labels = currentLabels.toArray(new String[0]);
    			boolean empty = true;
    			for (String l : labels) {
    				if (l != null && !"".equals(l.trim())) empty = false;
    			}
    			if (!empty) vertexAttributes.addReadOnly(Attribute.LABELS, StorageModel.STRING_ARRAY, labels);
    		}
    		currentPoints = currentColors = currentNormals = null;
    		currentLabels = null;
    		
            return;
        }

        if(qName.equals("points")) {
            return;
        }

        if(qName.equals("p")) {
            return;
        }

        if(qName.equals("lineSet")) {
        	if (currentEdges == null || currentEdges.size() == 0) return;
    		int[][] edges = currentEdges.toArray(new int[0][]);
        	edgeAttributes = new DataListSet(edges.length);
        	edgeAttributes.addReadOnly(Attribute.INDICES, StorageModel.INT_ARRAY_ARRAY, edges);
    		if (currentColors != null && !currentColors.isEmpty()) {
    			double[][] colors = currentColors.toArray(new double[0][]);
    			edgeAttributes.addReadOnly(Attribute.COLORS, StorageModel.DOUBLE_ARRAY_ARRAY, colors);
    		}
    		if (!currentLabels.isEmpty()) {
    			String[] labels = currentLabels.toArray(new String[0]);
    			boolean empty = true;
    			for (String l : labels) {
    				if (l != null && !"".equals(l.trim())) empty = false;
    			}
    			if (!empty) edgeAttributes.addReadOnly(Attribute.LABELS, StorageModel.STRING_ARRAY, labels);
    		}
    		
    		currentEdges = null;
    		currentColors = currentNormals = null;
    		currentLabels = null;

    		return;
        }
 
       if(qName.equals("lines")) {
           return;
        }

        if(qName.equals("l")) {
        	return;
        }

        if(qName.equals("faceSet")) {
        	if (currentFaces.size() == 0) return;
    		int[][] faces = currentFaces.toArray(new int[0][]);
    		faceAttributes = new DataListSet(faces.length);
    		faceAttributes.addReadOnly(Attribute.INDICES, StorageModel.INT_ARRAY_ARRAY, faces);
    		if (currentNormals != null && !currentNormals.isEmpty()) {
    			double[][] normals = currentNormals.toArray(new double[0][]);
    			faceAttributes.addReadOnly(Attribute.NORMALS, StorageModel.DOUBLE_ARRAY_ARRAY, normals);
    		}
    		if (currentColors != null && !currentColors.isEmpty()) {
    			double[][] colors = currentColors.toArray(new double[0][]);
    			faceAttributes.addReadOnly(Attribute.COLORS, StorageModel.DOUBLE_ARRAY_ARRAY, colors);
    		}
    		if (!currentLabels.isEmpty()) {
    			String[] labels = currentLabels.toArray(new String[0]);
    			boolean empty = true;
    			for (String l : labels) {
    				if (l != null && !"".equals(l.trim())) empty = false;
    			}
    			if (!empty) faceAttributes.addReadOnly(Attribute.LABELS, StorageModel.STRING_ARRAY, labels);
    		}

    		currentFaces = null;
    		currentColors = currentNormals = null;
    		currentLabels = null;

    		return;
        }

        if(qName.equals("faces")) {
        	return;
        }

        if(qName.equals("f")) {
        	return;
        }

        if(qName.equals("colors")) {
            return;
        }

        if(qName.equals("c")) {
            return;
        }

        if(qName.equals("normals")) {
            return;
        }

        if(qName.equals("n")) {
            return;
        }

    }
    
    LinkedList<char[]> chars = new LinkedList<char[]>();
    public void characters(char[] ch, int start, int length) throws SAXException {
		// QUICK FIX: sax parser sometimes splits char content into two calls of characters(..)
    	if (currentCharData == CharData.NONE) return;
    	char[] read = new char[length];
    	System.arraycopy(ch, start, read, 0, length);
    	chars.add(read);
    }

    public void charactersImpl() {
    	if (chars.isEmpty()) return;
    	char[] ch;
    	if (chars.size() > 1) {
    		int len=0;
    		for (int i=0; i < chars.size(); i++) {
    			len+=chars.get(i).length;
    		}
    		ch=new char[len];
    		for (int i=0, s=0; i < chars.size(); i++) {
    			char[] cs = chars.get(i);
				System.arraycopy(cs, 0, ch, s, cs.length);
    			s+=cs.length;
    		}
    	} else {
    		ch=chars.get(0);
    	}
    	chars.clear();
        String s = new String(ch);
        s=s.trim();

        switch (currentCharData) {
        case NONE:
        	return;
		case FACE:
		case LINE:
            String[] nums = s.split("\\s+");
			int[] data = new int[nums.length];
			for (int i = 0; i < data.length; i++) {
				data[i] = Integer.parseInt(nums[i]);
			}
			if (currentCharData == CharData.FACE) currentFaces.add(data);
			else {
				currentEdges.add(data);
			}
			return;
		case COLOR:
            nums = s.split("\\s+");
			double[] doubles = new double[nums.length];
			for (int i = 0; i < doubles.length; i++) {
				doubles[i] = Double.parseDouble(nums[i]);
			}
			for (int i = 0; i < doubles.length; i++) {
				doubles[i]/=255.;
			}
			currentColors.add(doubles);
            return;
		case NORMAL:
            nums = s.split("\\s+");
			doubles = new double[nums.length];
			for (int i = 0; i < doubles.length; i++) {
				doubles[i] = Double.parseDouble(nums[i]);
			}
			currentNormals.add(doubles);
            return;
		case POINT:
			if (currentPoints == null) return;
				nums = s.split("\\s+");
				doubles = new double[nums.length];
				for (int i = 0; i < doubles.length; i++) {
					doubles[i] = Double.parseDouble(nums[i]);
				}
			currentPoints.add(doubles);
            return;
		}
    }
}

}
