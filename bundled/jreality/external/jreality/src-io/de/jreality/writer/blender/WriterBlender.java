package de.jreality.writer.blender;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.io.Writer;
import java.nio.channels.Channels;
import java.nio.channels.WritableByteChannel;

import de.jreality.io.JrScene;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SceneGraphNode;
import de.jreality.util.SceneGraphUtility;
import de.jreality.writer.SceneWriter;

/**
 * 
 * @author Stefan Sechelmann, Thilo Rörig
 *
 */
public class WriterBlender implements SceneWriter {

	private File writeBlenderSceneTmp(JrScene scene) throws IOException {
		BlenderConnection c = new BlenderConnection();
		File f = File.createTempFile("blenderExport", ".blend");
		f.deleteOnExit();
		c.writeFile(scene, f);
		return f;
	}
	
	@Override
	public void writeScene(JrScene scene, OutputStream out) throws IOException {
		File sceneFile = writeBlenderSceneTmp(scene);
		WritableByteChannel wbc = Channels.newChannel(out);
		FileInputStream fin = new FileInputStream(sceneFile);
		fin.getChannel().transferTo(0, Long.MAX_VALUE, wbc);
		fin.close();
		out.close();
	}

	@Override
	public void writeScene(JrScene scene, Writer out) throws IOException, UnsupportedOperationException {
		throw new UnsupportedOperationException();
	}

	@Override
	public void write(SceneGraphNode c, OutputStream out) throws IOException {
		SceneGraphComponent root = null;
		if (c instanceof SceneGraphComponent) {
			root = (SceneGraphComponent) c;
		} else {
			root = new SceneGraphComponent();
			SceneGraphUtility.addChildNode(root, c);
		}
		JrScene scene = new JrScene(root);
		writeScene(scene, out);
	}

}
