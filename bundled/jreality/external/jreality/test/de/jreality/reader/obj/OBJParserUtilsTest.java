package de.jreality.reader.obj;

import static de.jreality.reader.obj.OBJParserUtils.parseDoubleArray;
import static de.jreality.reader.obj.OBJParserUtils.parseIntArray;
import static de.jreality.reader.obj.OBJParserUtils.parseStringArray;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.Reader;
import java.io.StreamTokenizer;
import java.io.StringReader;
import java.util.LinkedList;
import java.util.List;

import org.junit.Assert;
import org.junit.Test;

public class OBJParserUtilsTest {

	@Test
	public void testParseLine() throws IOException {
		OBJVertex v1 = new OBJVertex(1,0,0);
		OBJVertex v2 = new OBJVertex(2,0,0);
		OBJVertex v3 = new OBJVertex(3,0,0);
		List<OBJVertex> list = new LinkedList<OBJVertex>();
		list.add(v1);
		list.add(v2);
		list.add(v3);
		String str = "1 2 \\ 3";
		Reader r = new BufferedReader(new StringReader(str));
		StreamTokenizer st = new StreamTokenizer(r);
		OBJParserUtils.globalSyntax(st);
		List<OBJVertex> line = OBJParserUtils.parseVertexList(st);
		int i = 0;
		for(OBJVertex v : line) {
			Assert.assertTrue(v.equalIndices(list.get(i++)));
		}
	}

	@Test
	public void testParseLine_N() throws IOException {
		OBJVertex v1 = new OBJVertex(1,0,1);
		OBJVertex v2 = new OBJVertex(2,0,2);
		OBJVertex v3 = new OBJVertex(3,0,3);
		List<OBJVertex> list = new LinkedList<OBJVertex>();
		list.add(v1);
		list.add(v2);
		list.add(v3);
		String str = "1//1 2//2 3//3";
		Reader r = new BufferedReader(new StringReader(str));
		StreamTokenizer st = new StreamTokenizer(r);
		OBJParserUtils.globalSyntax(st);
		List<OBJVertex> line = OBJParserUtils.parseVertexList(st);
		int i = 0;
		for(OBJVertex v : line) {
			Assert.assertTrue(v.equalIndices(list.get(i++)));
		}
	}

	@Test
	public void testParseOBJVertex() throws IOException {
		OBJVertex v = new OBJVertex(1,2,3);
		String str = v.toString();
		Reader r = new BufferedReader(new StringReader(str));
		StreamTokenizer st = new StreamTokenizer(r);
		OBJParserUtils.globalSyntax(st);
		OBJVertex nv = OBJParserUtils.parseVertex(st);
		Assert.assertTrue(v.equalIndices(nv));
	}

	@Test
	public void testParseVertex_TN() throws IOException {
		OBJVertex v = new OBJVertex(1,2,3);
		String str = v.toString();
		Reader r = new BufferedReader(new StringReader(str));
		StreamTokenizer st = new StreamTokenizer(r);
		OBJParserUtils.globalSyntax(st);
		OBJVertex nv = OBJParserUtils.parseVertex(st);
		Assert.assertTrue(v.equalIndices(nv));
	}

	@Test
	public void testParseDoubleArray() throws IOException {
		double[] inCoords = {0.1, 0.2, 0.3};
		String str = "0.1 0.2 0.3";
		Reader r = new BufferedReader(new StringReader(str));
		StreamTokenizer st = createOBJStreamTokenizer(r);
		double[] coords = parseDoubleArray(st);
		Assert.assertArrayEquals(inCoords, coords, 1E-10);
	}

	@Test
	public void testParseDoubleArray_Exp() throws IOException {
		double[] inCoords = {-0.1, 0.2, 0.3};
		String str = "-1E-1 +2E-1 0.3";
		Reader r = new BufferedReader(new StringReader(str));
		StreamTokenizer st = createOBJStreamTokenizer(r);
		double[] coords = parseDoubleArray(st);
		Assert.assertArrayEquals(inCoords, coords, 1E-10);
	}

	@Test
	public void testParseDoubleArray_Negative() throws IOException {
		double[] inCoords = {-0.1, 0.2, 0.3};
		String str = "-0.1 0.2 0.3";
		Reader r = new BufferedReader(new StringReader(str));
		StreamTokenizer st = createOBJStreamTokenizer(r);
		double[] coords = parseDoubleArray(st);
		Assert.assertArrayEquals(inCoords, coords, 1E-10);
	}

	@Test
	public void testParseDoubleArray_Plus() throws IOException {
		double[] inCoords = {-0.1, 0.2, 0.3};
		String str = "-0.1 +0.2 0.3";
		Reader r = new BufferedReader(new StringReader(str));
		StreamTokenizer st = createOBJStreamTokenizer(r);
		double[] coords = parseDoubleArray(st);
		Assert.assertArrayEquals(inCoords, coords, 1E-10);
	}

	@Test
	public void testParseIntArray() throws IOException {
		double[] inIndices = {-1, 2, 3};
		String str = "-1 2 3";
		Reader r = new BufferedReader(new StringReader(str));
		StreamTokenizer st = createOBJStreamTokenizer(r);
		List<Integer> indices = parseIntArray(st);
		int i = 0;
		for(int index : indices) {
			Assert.assertEquals(inIndices[i++],index,1E-10);
		}
	}

	@Test
	public void testParseStringArray() throws IOException {
		String[] list = {"name1", "name2", "name3"}; 
		String str = "name1 name2 name3 ";
		Reader r = new BufferedReader(new StringReader(str));
		StreamTokenizer st = createOBJStreamTokenizer(r);
		List<String> strings = parseStringArray(st);
		int i = 0;
		for(String s : strings) {
			Assert.assertTrue(s.equals(list[i++]));
		}
	}

	private StreamTokenizer createOBJStreamTokenizer(Reader r) {
		StreamTokenizer st = new StreamTokenizer(r);
		return OBJParserUtils.globalSyntax(st);
	}

	@Test
	public void testParseLine_T() throws IOException {
		OBJVertex v1 = new OBJVertex(1,1,0);
		OBJVertex v2 = new OBJVertex(2,2,0);
		OBJVertex v3 = new OBJVertex(3,3,0);
		List<OBJVertex> list = new LinkedList<OBJVertex>();
		list.add(v1);
		list.add(v2);
		list.add(v3);
		String str = "1/1 2/2 3/3";
		Reader r = new BufferedReader(new StringReader(str));
		StreamTokenizer st = createOBJStreamTokenizer(r);
		List<OBJVertex> line = OBJParserUtils.parseVertexList(st);
		int i = 0;
		for(OBJVertex v : line) {
			Assert.assertTrue(v.equalIndices(list.get(i++)));
		}
	}

	@Test
	public void testParseLine_TN() throws IOException {
		OBJVertex v1 = new OBJVertex(1,1,1);
		OBJVertex v2 = new OBJVertex(2,2,2);
		OBJVertex v3 = new OBJVertex(3,3,3);
		List<OBJVertex> list = new LinkedList<OBJVertex>();
		list.add(v1);
		list.add(v2);
		list.add(v3);
		String str = "1/1/1 2/2/2 3/3/3";
		Reader r = new BufferedReader(new StringReader(str));
		StreamTokenizer st = createOBJStreamTokenizer(r);
		List<OBJVertex> line = OBJParserUtils.parseVertexList(st);
		int i = 0;
		for(OBJVertex v : line) {
			Assert.assertTrue(v.equalIndices(list.get(i++)));
		}
	}

	@Test
	public void testParseVertex_N() throws IOException {
		OBJVertex v = new OBJVertex(1,0,2);
		String str = v.toString();
		Reader r = new BufferedReader(new StringReader(str));
		StreamTokenizer st = createOBJStreamTokenizer(r);
		OBJVertex nv = OBJParserUtils.parseVertex(st);
		Assert.assertTrue(v.equalIndices(nv));
	}

	@Test
	public void testParseVertex_T() throws IOException {
		OBJVertex v = new OBJVertex(1,2,0);
		String str = v.toString();
		Reader r = new BufferedReader(new StringReader(str));
		StreamTokenizer st = createOBJStreamTokenizer(r);
		OBJVertex nv = OBJParserUtils.parseVertex(st);
		Assert.assertTrue(v.equalIndices(nv));
	}

}
