package de.jreality.tutorial.reader;

import java.io.IOException;
import java.net.URL;

import de.jreality.plugin.JRViewer;
import de.jreality.plugin.JRViewer.ContentType;
import de.jreality.reader.Readers;
import de.jreality.scene.SceneGraphComponent;

/**
 * Example how to register a custom reader for a certain format, including the
 * corresponding file endings.
 * 
 * @author Steffen Weissman
 *
 */
public class ReaderExample {

	public static void main(String[] args) {
		
		// register the reader class for the DEMO-format
		Readers.registerReader("DEMO", DemoReader.class);
		// register the file ending .demo for files containing DEMO-format data
		Readers.registerFileEndings("DEMO", "demo");
		
		// load the sample file:
		//
		// This is not needed, after registering the reader one can load
		// DEMO-files from the File-Menu: File->Load
		//
		URL fileUrl = ReaderExample.class.getResource("samplefile.demo");
		SceneGraphComponent content = null;
		try {
			content = Readers.read(fileUrl);
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		
		JRViewer v = new JRViewer();
		v.addBasicUI();
		v.addContentSupport(ContentType.CenteredAndScaled);
		v.addContentUI();
		v.setContent(content);
		v.startup();
	}
}
