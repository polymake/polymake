package de.jreality.reader.obj;

import java.util.LinkedList;
import java.util.List;

import org.junit.Assert;
import org.junit.Test;

public class OBJIndexFactoryTest {

	@Test
	public void testCompareIndex() {
		Assert.assertEquals(OBJIndexFactory.compareIndex(0,1), 0);
		Assert.assertEquals(OBJIndexFactory.compareIndex(1,1), 0);
		Assert.assertEquals(OBJIndexFactory.compareIndex(1,0), 0);
		Assert.assertEquals(OBJIndexFactory.compareIndex(1,2), -1);
		Assert.assertEquals(OBJIndexFactory.compareIndex(15,12), 3);
	}

	@Test
	public void testGetID_MergeTextureAndNormals() {
		List<OBJVertex> points = new LinkedList<OBJVertex>();
		OBJVertex v1 = new OBJVertex(1,1,1);
		points.add(v1);
		OBJVertex v2 = new OBJVertex(1,0,0);
		points.add(v2);
		OBJVertex v3 = new OBJVertex(1,2,0);
		points.add(v3);
		OBJVertex v4 = new OBJVertex(2,2,0);
		points.add(v4);
		
		OBJIndexFactory vd = new OBJIndexFactory(points,null,null,false);
		Assert.assertEquals(0,vd.getId(v1).intValue());
		Assert.assertEquals(0,vd.getId(v2).intValue());
		Assert.assertEquals(0,vd.getId(v3).intValue());
		Assert.assertEquals(1,vd.getId(v4).intValue());
	}

	@Test
	public void testGetID_SplitTextureAndNormals() {
		List<OBJVertex> points = new LinkedList<OBJVertex>();
		OBJVertex v1 = new OBJVertex(1,1,1);
		points.add(v1);
		OBJVertex v2 = new OBJVertex(1,0,0);
		points.add(v2);
		OBJVertex v3 = new OBJVertex(1,2,0);
		points.add(v3);
		OBJVertex v4 = new OBJVertex(2,2,0);
		points.add(v4);
		
		OBJIndexFactory vd = new OBJIndexFactory(points,null,null,true);
		Assert.assertEquals(0,vd.getId(v1).intValue());
		Assert.assertEquals(0,vd.getId(v2).intValue());
		Assert.assertEquals(1,vd.getId(v3).intValue());
		Assert.assertEquals(2,vd.getId(v4).intValue());
	}

}
