package de.jreality.writer.blender;

import java.io.BufferedInputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.StringWriter;
import java.io.Writer;
import java.nio.channels.Channels;
import java.nio.channels.FileChannel;
import java.nio.channels.ReadableByteChannel;
import java.util.ArrayList;
import java.util.List;
import java.util.logging.Logger;
import java.util.prefs.Preferences;

import de.jreality.io.JrScene;
import de.jreality.writer.WriterJRS;

/**
 * 
 * @author Stefan Sechelmann, Thilo Rörig
 *
 */
public class BlenderConnection {

	private static Logger
		log = Logger.getLogger(BlenderConnection.class.getName());
	private static Preferences 
		preferences = Preferences.userNodeForPackage(BlenderConnection.class);
	private static File
		blenderApp = null,
		rendererScript = null;
	private Writer
		stdoutRedirect = null;
	
	static {
		String blenderExecutable = preferences.get("blenderExecutable", "blender");
		blenderApp = new File(blenderExecutable);
	}
	
	public BlenderConnection() {
	}
	
	public static void setBlenderExecutable(File blender) {
		BlenderConnection.blenderApp = blender;
		preferences.put("blenderExecutable", blender.getAbsolutePath());
		try {
			preferences.flush();
		} catch (Exception e) {
			log.warning("Could not flush preferences: " + e);
		}
	}
	
	protected void invokeBlender(File sceneFile, File outBlenderFile, File outImage) throws IOException {
		// run renderer
		File script = null;
		try {
			script = getRendererScript();
		} catch (IOException e1) {
			log.warning("could not write blender renderer script: " + e1.getMessage());
			throw e1;
		}
		List<String> argList = new ArrayList<String>();
		argList.add("./blender");
		argList.add("--background");
		argList.add("--factory-startup");
		argList.add("--render-format");
		argList.add("PNG");
		argList.add("--python");
		argList.add(script.toString());
		argList.add("--");
		if (outImage != null) {
			argList.add("--render=" + outImage.getAbsolutePath());
		}
		if (outBlenderFile != null) {
			argList.add("--save=" + outBlenderFile.getAbsolutePath());
		}
		argList.add("--file=" + sceneFile.getAbsolutePath());
		String[] argArray = argList.toArray(new String[argList.size()]);
		try {
			File dir = blenderApp.getParentFile();
			Process p = Runtime.getRuntime().exec(argArray, new String[]{}, dir);
			InputStream in = new BufferedInputStream(p.getInputStream());
			InputStream err = new BufferedInputStream(p.getErrorStream());
			int bIn = 0;
			while ((bIn = in.read()) != -1) {
				System.out.write(bIn);
				if (stdoutRedirect != null) {
					stdoutRedirect.write(bIn);
				}
			}
			StringWriter sw = new StringWriter();
			while ((bIn = err.read()) != -1) {
				System.err.write(bIn);
				sw.write(bIn);
			}
			p.waitFor();
			String errString = sw.getBuffer().toString();
			if (!errString.isEmpty()) {
				throw new RuntimeException(errString);
			}
		} catch (InterruptedException e) {
			e.printStackTrace();
		} finally {
			if (stdoutRedirect != null) {
				stdoutRedirect.close();
			}
		}
	}
	
	protected File writeScene(JrScene scene) throws IOException {
		File sceneFile = File.createTempFile("jrealityBlenderExportScene", ".jrs");
		sceneFile.deleteOnExit();
		OutputStream sceneOut = new FileOutputStream(sceneFile); 
		WriterJRS jrsWriter = new WriterJRS();
		jrsWriter.writeScene(scene, sceneOut);
		sceneOut.close();
		return sceneFile;
	}
	
	public void writeFile(JrScene scene, File f) throws IOException {
		File sceneFile = writeScene(scene);
		invokeBlender(sceneFile, f, null);
	}
	
	public void renderImage(JrScene scene, File f) throws IOException {
		File sceneFile = writeScene(scene);
		invokeBlender(sceneFile, null, f);
	}
	
	private File getRendererScript() throws IOException {
		if (rendererScript != null) {
			return rendererScript;
		}
		rendererScript = File.createTempFile("jrealityBlenderRenderer", "py");
		rendererScript.deleteOnExit();
		ReadableByteChannel rbc = Channels.newChannel(getClass().getResourceAsStream("renderer.py"));
		FileOutputStream fin = new FileOutputStream(rendererScript);
		FileChannel outChannel = fin.getChannel();
		outChannel.transferFrom(rbc, 0, Long.MAX_VALUE);
		outChannel.close();
		fin.close();
		return rendererScript;
	}
	
	public void setStdoutRedirect(Writer stdoutRedirect) {
		this.stdoutRedirect = stdoutRedirect;
	}
	
}
