package de.jreality.tutorial.misc;

import java.io.IOException;

import de.jreality.plugin.JRViewer;
import de.jreality.reader.Readers;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.util.Input;

public class FileReaderExample {

  public static void main(String[] args) throws IOException {
    // see the documentation of Input.getInput(String resource)
    //
    // To read .bsh files, you need beanshell in the classpath, to
    // read .jrs files, you need xstream.jar and xpp3.jar.
    //
	if (args.length > 0) {
		SceneGraphComponent scp = Readers.read(Input.getInput(args[0]));
		JRViewer.display(scp);
	} else {
		System.out.println("usage: java de.jreality.tutorial.FileReaderExample <3d-data-file>");
	}
  }

}
