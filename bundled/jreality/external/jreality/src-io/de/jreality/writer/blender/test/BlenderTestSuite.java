package de.jreality.writer.blender.test;

import java.io.IOException;
import java.net.URL;

import org.junit.Assert;
import org.junit.Test;

import de.jreality.reader.ReaderJRS;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.writer.blender.WriterBlender;
import de.smrj.tcp.util.ByteBufferOutputStream;

public class BlenderTestSuite {

	private void exportScene(String fileName) throws IOException {
		ReaderJRS r = new ReaderJRS();
		URL inURL = getClass().getResource(fileName);
		SceneGraphComponent c = r.read(inURL);
		WriterBlender w = new WriterBlender();
		ByteBufferOutputStream out = new ByteBufferOutputStream();
		try {
			w.write(c, out);
		} catch (Exception e) {
			Assert.fail(e.getMessage());
		}
	}
	
	@Test
	public void testGeometry01() throws Exception {
		exportScene("geometry01.jrs");
	}
	
//	@Test
//	public void testGeometry02() throws Exception {
//		exportScene("geometry02.jrs");
//	}

//	@Test
//	public void testGeometry03() throws Exception {
//		exportScene("geometry03.jrs");
//	}
	
	@Test
	public void testManyFeatures01() throws Exception {
		exportScene("manyFeatures01.jrs");
	}
}
