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

package de.tuberlin.polymake.common.io;

import java.io.IOException;
import java.util.Iterator;
import java.util.Properties;
import java.util.StringTokenizer;
import java.util.Vector;

import de.tuberlin.polymake.common.SelectorThread;
import de.tuberlin.polymake.common.SharedMemoryMatrixException;
import de.tuberlin.polymake.common.geometry.PointSet;
import de.tuberlin.polymake.common.geometry.PolymakePoint;

/**
 * A simple line parser.
 * 
 * @author Thilo Rörig
 */
public class SimpleGeometryParser extends PolymakeParser {

	public SimpleGeometryParser(String[] params) {
		super(params);
	}

	/**
	 * first line of reader: n <name> second line of reader: # <n_points>
	 */
	public void parse(java.io.BufferedReader in, PointSet pointSet) throws IOException, SharedMemoryMatrixException {

		String line;
		error = null;
		warning = null;
		Vector points = new Vector();
		boolean end_flag = false;
		clearParams();
		while (!end_flag && (line = in.readLine()) != null) {
			if (System.getProperty("polymake.debug") != null
					&& System.getProperty("polymake.debug").equalsIgnoreCase(
							"max")) {
				SelectorThread.newErr.println(line);
			}
			line.trim();
			switch (line.charAt(0)) {
			case 'n': {
				name = line.substring(2);
				pointSet.setName(name);
				break;
			}
//			case '#': {
//				int n_points = Integer.parseInt(line.substring(2));
//				break;
//			}
			case 'e': {
				error = line.substring(2);
				break;
			}
			case 'w': {
				warning = line.substring(2);
				break;
			}
			case 'p': {
				PolymakePoint point = parsePoint(line.substring(2));
				points.add(point);
				break;
			}
			case 'P': {
				int smmKey = Integer.parseInt(line.substring(2));
				if(pointSet.getSMM() == null) {
					pointSet.initSMM(smmKey);
				}
				pointSet.readFromSMM();
				break;
			}
			/*
			 * case 'l': PiVector polygon = parsePolygon(line.substring(2));
			 * m_geom.addPolygon(polygon); break;
			 */
			case 'f': {
				parameters.setProperty("facet", line.substring(2));
				break;
			}
			case 's': {
				StringTokenizer st = new StringTokenizer(line.substring(2));
				String key = st.nextToken();
				if (parameters.keySet().contains(key)) {
					parameters.setProperty(key, st.nextToken());
				}
				break;
			}
			case 'i': {
				StringTokenizer st = new StringTokenizer(line.substring(2));
				String key = st.nextToken();
				if (parameters.keySet().contains(key)) {
					iparameters.setProperty(key, st.nextToken());
				}
				break;
			}
			case 'x': {
				end_flag = true;
				break;
			}
			default:
			}
		}
		// if points came via "p x1 x2 x3\n"
		if(points.size() != 0) {
			PolymakePoint[] pts = new PolymakePoint[points.size()];
			Iterator it = points.iterator();
			for (int i = 0; i < pts.length; ++i) {
				PolymakePoint pt = (PolymakePoint) it.next();
				pts[i] = pt;
			}
			pointSet.setPoints(pts);
		} 
	}

	private void clearParams() {
		for (Iterator it = parameters.keySet().iterator(); it.hasNext();) {
			String param = (String) (it.next());
			parameters.setProperty(param, "null");
		}
	}

	public Vector getFacet() {
		return parseFacet(parameters.getProperty("facet"));
	}

	/**
	 * Parses a <code>String</code> into a <code>PolymakePoint</code>
	 * containing coordinates and possibly a label.
	 * 
	 * @param str
	 *            the string to be parsed
	 * @return a <code>PolymakePoint</code> containing the coordinates and
	 *         possibly a label
	 * @exception NumberFormatException
	 *                if there are non-double coordinates
	 */
	public static PolymakePoint parsePoint(String str)
			throws NumberFormatException {
		StringTokenizer tokenizer = new StringTokenizer(str);
		Vector point = new Vector();
		String label = null;
		while (tokenizer.hasMoreTokens()) {
			String token = tokenizer.nextToken();
			if (token.equals("l")) {
				label = tokenizer.nextToken();
			} else {
				point.add(new Double(token));
			}
		}

		double[] coords = new double[point.size()];
		int i = 0;
		Iterator it = point.iterator();
		while (it.hasNext()) {
			double value = ((Double) it.next()).doubleValue();
			coords[i++] = value;
		}
		return new PolymakePoint(coords, label);
	}

	/**
	 * Parses a <code>String</code> into a <code>PiVector</code> the
	 * vertices of a polygon
	 * 
	 * @param str
	 *            the string to be parsed
	 * @return a <code>PiVector</code> containing the vertices of a polygon
	 * @exception NumberFormatException
	 *                if there are non-integer vertices
	 */
	/*
	 * public static PiVector parsePolygon(String str) throws
	 * NumberFormatException { StringTokenizer tokenizer = new
	 * StringTokenizer(str); PiVector polygon = new PiVector(); while
	 * (tokenizer.hasMoreTokens()) { String token = tokenizer.nextToken();
	 * polygon.addEntry(Integer.parseInt(token)); } return polygon; }
	 */

	/**
	 * Parses a <code>String</code> into a <code>Set</code> containing its
	 * vertices.
	 * 
	 * @param str
	 *            the string to be parsed
	 * @return a <code>Vector</code> containing the vertices
	 * @exception NumberFormatException
	 *                if there are non-int vertices
	 */
	public static Vector parseFacet(String str) throws NumberFormatException {
		StringTokenizer tokenizer = new StringTokenizer(str);
		Vector facet = new Vector();
		while (tokenizer.hasMoreTokens()) {
			facet.add(new Integer(tokenizer.nextToken()));
		}
		return facet;
	}

	public static String write(PointSet pointSet, Properties params) {
		String msg = new String();
		if (pointSet != null) {
			msg += "n " + pointSet.getName() + "\n";
			try {
				if(pointSet.getSMM() != null) {
					pointSet.getSMM().setCoords(pointSet);
					msg += "P " + pointSet.getSMM().getAddr() + "\n";
				} else {
					PolymakePoint[] points = pointSet.getPoints();
					for (int i = 0; i < points.length; ++i) {
						double[] vertex = points[i].getCoords();
						msg += "p";
						for (int j = 0; j < vertex.length; ++j) {
							msg += " " + vertex[j];
						}
						if (points[i].getLabel() != null) {
							msg += " l " + points[i].getLabel();
						}
						msg += "\n";
					}
				}
			} catch (SharedMemoryMatrixException e) {
				e.printStackTrace(SelectorThread.newErr);
			}
		}

		if (params != null) {
			for (Iterator it = params.keySet().iterator(); it.hasNext();) {
				String param = (String) it.next();
				String value = params.getProperty(param);
				if (value != null)
					msg += "s " + param + " " + value + "\n";
			}
		}
		msg += "x\n";
		return msg;
	}

	public static String write(String name, Properties params) {
		String msg = new String();
		msg += "n " + name + "\n";

		if (params != null) {
			for (Iterator it = params.keySet().iterator(); it.hasNext();) {
				String param = (String) it.next();
				String value = params.getProperty(param);
				if (value != null)
					msg += "s " + param + " " + value + "\n";
			}
		}

		msg += "x\n";
		return msg;
	}

	
	public static String write(String name, int vertexIndex) {
		String msg = new String();
		msg += "n " + name + "\n";
		msg += "p " + vertexIndex + "\n";
		msg += "x\n";
		return msg;
	}

	public static String write(String name, String command, double value) {
		String msg = new String();
		msg += "n " + name + "\n";
		msg += "s " + command + " " + value + "\n";
		msg += "x\n";
		return msg;
	}

	public static String write(PointSet pointSet, Vector marked,
			Properties params) {
		String msg = new String();
		try{
			if (pointSet != null) {
			msg += "n " + pointSet.getName() + "\n";
			if(pointSet.getSMM() != null) {
				pointSet.writeToSMM();
				msg += "P " + pointSet.getSMM().getAddr() + "\n";
			} else {
				PolymakePoint[] points = pointSet.getPoints();
				for (int i = 0; i < points.length; ++i) {
					double[] vertex = points[i].getCoords();
					msg += "p";
					for (int j = 0; j < vertex.length; ++j) {
						msg += " " + vertex[j];
					}
					if (points[i].getLabel() != null) {
						msg += " l " + points[i].getLabel();
					}
					msg += "\n";
				}
			}
			msg += "f ";
			Iterator it = marked.iterator();
			while (it.hasNext()) {
				msg += (String) it.next() + " ";
			}
			msg += "\n";
		}

		if (params != null) {
			for (Iterator it = params.keySet().iterator(); it.hasNext();) {
				String param = (String) it.next();
				String value = params.getProperty(param);
				if (value != null)
					msg += "s " + param + " " + value + "\n";
			}
		}
		} catch(SharedMemoryMatrixException ex) {
			ex.printStackTrace(SelectorThread.newErr);
		}
		msg += "x\n";
		return msg;
	}

	public static String writeFacet(String name, Vector verts) {
		String msg = new String();
		msg += "n " + name + "\n";
		msg += "f ";
		Iterator it = verts.iterator();
		while (it.hasNext()) {
			msg += (String) it.next() + " ";
		}
		msg += "\n" + "x\n";
		return msg;
	}

    public static String writeTransformationMatrix(double[] matrix) {
        String trs = "T";
        for (int i = 0; i < 16; ++i) {
            trs = trs + " " + matrix[i];
        }
        return trs;
    }
}
