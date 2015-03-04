package de.jreality.reader.obj;

import static java.io.StreamTokenizer.TT_EOF;
import static java.io.StreamTokenizer.TT_EOL;
import static java.io.StreamTokenizer.TT_NUMBER;

import java.io.IOException;
import java.io.StreamTokenizer;
import java.util.ArrayList;
import java.util.LinkedList;
import java.util.List;

import de.jreality.reader.ParserUtil;

public class OBJParserUtils {

	public static double[] parseDoubleArray(StreamTokenizer st) throws IOException {
		List<Double> cList = new LinkedList<Double>();
		st.nextToken();
		while (st.ttype == TT_NUMBER || st.ttype == '\\' || st.sval.startsWith("+")) {
			if (st.ttype == '\\') {
				st.nextToken(); // the EOL
				st.nextToken(); // continue parsing in the next line
				continue;
			} else if(st.ttype == TT_NUMBER) {
				st.pushBack();
				cList.add(ParserUtil.parseNumber(st));
			} else if(st.sval.startsWith("+")) {
				cList.add(Double.parseDouble(st.sval.replace("+", "")));
			}
			st.nextToken();
			if(st.ttype == TT_EOF || st.ttype == TT_EOL) {
				break;
			}
		}
		st.pushBack();
		double[] coords = new double[cList.size()];
		for (int i = 0; i < coords.length; i++) {
			coords[i] = cList.get(i);
		}
		return coords;
	}

	public static List<Integer> parseIntArray(StreamTokenizer st) throws IOException {
		List<Integer> integers = new LinkedList<Integer>();
		
		st.nextToken();
		while(st.ttype != TT_EOL && st.ttype != TT_EOF) {
			if (st.ttype == '\\') {
				st.nextToken(); // the EOL
				st.nextToken(); // continue parsing in the next line
				continue;
			}
			if(st.ttype == StreamTokenizer.TT_NUMBER) {
				integers.add((int) st.nval);
			}
			st.nextToken();
		}
		return integers;
	}

	public static List<String> parseStringArray(StreamTokenizer st) throws IOException {
		List<String> groupNames = new LinkedList<String>();
		
		st.nextToken();
		while(st.ttype != TT_EOL && st.ttype != TT_EOF) {
			if (st.ttype == '\\') {
				st.nextToken(); // the EOL
				st.nextToken(); // continue parsing in the next line
				continue;
			} 
			groupNames.add(st.sval);
			st.nextToken();
		}
		return groupNames;
	}

	public static OBJVertex parseVertex(StreamTokenizer st) throws IOException {
		OBJVertex v = new OBJVertex();
		st.nextToken();
		v.setVertexIndex((int) st.nval);
		st.nextToken();
		if (st.ttype == '/') {
			st.nextToken();
			if (st.ttype == StreamTokenizer.TT_NUMBER) {
				v.setTextureIndex((int) st.nval);
				st.nextToken();
			} 
			if(st.ttype == '/') {
				st.nextToken();
				if (st.ttype == StreamTokenizer.TT_NUMBER) {
					v.setNormalIndex((int) st.nval);
				}
			} else {
				st.pushBack();
			}
		} else {
			st.pushBack();
		}
		return v;
	}

	public static List<OBJVertex> parseVertexList(StreamTokenizer st) throws IOException {
		ArrayList<OBJVertex> v = new ArrayList<OBJVertex>(3);
		st.nextToken();
		while (st.ttype != TT_EOL && st.ttype != TT_EOF) {
			if (st.ttype == '\\') {
				st.nextToken(); // the EOL
				st.nextToken(); // continue parsing in the next line
				continue;
			} 
			else {
				st.pushBack();
				v.add(parseVertex(st));
			}
			st.nextToken();
		}
		return v;
	}

	public static StreamTokenizer filenameSyntax(StreamTokenizer st) {
		st.resetSyntax();
		st.eolIsSignificant(true);
		st.wordChars('0', '9');
		st.wordChars('A', 'Z');
		st.wordChars('a', 'z');
		st.wordChars('_', '_');
		st.wordChars('.', '.');
		st.wordChars('-', '-');
		st.wordChars('+', '+');
		st.wordChars('\u00A0', '\u00FF');
		st.whitespaceChars('\u0000', '\u0020');
		st.commentChar('#');
		st.ordinaryChar('/');
		st.parseNumbers();
		return st;
	}

	public static StreamTokenizer globalSyntax(StreamTokenizer st) {
		st.resetSyntax();
		st.eolIsSignificant(true);
		st.wordChars('0', '9');
		st.wordChars('A', 'Z');
		st.wordChars('a', 'z');
		st.wordChars('_', '_');
		st.wordChars('.', '.');
		st.wordChars('-', '-');
		st.wordChars('+', '+');
		st.wordChars('\u00A0', '\u00FF');
		st.whitespaceChars('\u0000', '\u0020');
		st.commentChar('#');
		st.ordinaryChar('/');
		st.parseNumbers();
		return st;
	}

}
