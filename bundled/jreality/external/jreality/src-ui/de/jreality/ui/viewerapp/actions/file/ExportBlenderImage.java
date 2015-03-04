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

package de.jreality.ui.viewerapp.actions.file;

import static javax.swing.JOptionPane.ERROR_MESSAGE;
import static javax.swing.JOptionPane.INFORMATION_MESSAGE;
import static javax.swing.JOptionPane.YES_NO_CANCEL_OPTION;

import java.awt.Component;
import java.awt.EventQueue;
import java.awt.Font;
import java.awt.GridLayout;
import java.awt.Window;
import java.awt.event.ActionEvent;
import java.awt.image.BufferedImage;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.PrintStream;
import java.io.StringWriter;
import java.io.Writer;
import java.lang.reflect.InvocationTargetException;
import java.util.logging.Level;
import java.util.logging.Logger;

import javax.imageio.ImageIO;
import javax.swing.Icon;
import javax.swing.ImageIcon;
import javax.swing.JDialog;
import javax.swing.JFileChooser;
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JOptionPane;
import javax.swing.JScrollPane;
import javax.swing.JTextArea;
import javax.swing.SwingUtilities;
import javax.swing.text.Document;

import de.jreality.io.JrScene;
import de.jreality.scene.Viewer;
import de.jreality.ui.viewerapp.FileLoaderDialog;
import de.jreality.ui.viewerapp.actions.AbstractJrAction;
import de.jreality.writer.blender.BlenderConnection;

/**
 * 
 * @author Stefan Sechelmann, Thilo Rörig
 * 
 */
public class ExportBlenderImage extends AbstractJrAction {

	private static final long 
		serialVersionUID = 1L;
	private Viewer 
		viewer = null;
	private static JFileChooser
		executableChooser = new JFileChooser();
	private Logger
		log = Logger.getLogger(ExportBlenderImage.class.getName());

	static {
		executableChooser.setAcceptAllFileFilterUsed(true);
		executableChooser.setMultiSelectionEnabled(false);
		executableChooser.setDialogTitle("Select blender executable");
		executableChooser.setFileSelectionMode(JFileChooser.FILES_ONLY);
	}
	
	public ExportBlenderImage(String name, Viewer viewer, Component parentComp) {
		super(name, parentComp);
		if (viewer == null) {
			throw new IllegalArgumentException("Viewer is null!");
		}
		this.viewer = viewer;
		setShortDescription("Save scene as a U3D file");
	}

	@Override
	public void actionPerformed(ActionEvent e) {
		File file = FileLoaderDialog.selectTargetFile(parentComp, "png", "PNG Image Files");
		if (file == null) {
			return;
		}
		try {
			renderSceneToFile(file, viewer);
		} catch (IOException ioe) {
			log.log(Level.WARNING, ioe.getMessage(), ioe);
		}
	}

	private void renderSceneToFile(File file, Viewer viewer) throws FileNotFoundException, IOException {
		Window w = SwingUtilities.getWindowAncestor(parentComp);
		JDialog f = new JDialog(w, "Blender Renderer: " + file.getName());
		f.setLayout(new GridLayout());
		JTextArea progressArea = new JTextArea("Starting Blender Renderer...\n");
		JScrollPane progressScroller = new JScrollPane(progressArea);
		JLabel imageLabel = new JLabel();
		Font textFont = new Font("Arial", Font.PLAIN, 10);
		progressArea.setFont(textFont);
		progressArea.setEditable(false);
		f.add(progressScroller);
		f.setSize(800, 400);
		f.setLocationRelativeTo(w);
		f.setVisible(true);
		f.setDefaultCloseOperation(JFrame.DISPOSE_ON_CLOSE);
		
		StringWriter sw = new StringWriter();
		RenderJob job = new RenderJob(file, sw);
		job.start();
		
		doRenderProgress(file, f, imageLabel, progressArea, progressScroller, sw, job);
	}

	private void doRenderProgress(
		final File file, 
		final JDialog f, 
		final JLabel imageLabel,
		final JTextArea progressArea,
		final JScrollPane progressScroller, 
		final StringWriter sw, 
		final RenderJob job
	) throws IOException {
		Thread progressJob = new Thread() {
			@Override
			public void run() {
				setName("Blender Renderer Progress");
				int len = 0;
				while (job.isAlive()) {
					int newLen = sw.getBuffer().length();
					if (len != newLen) {
						char[] newChars = new char[newLen - len];
						sw.getBuffer().getChars(len, newLen, newChars, 0);
						progressArea.append(new String(newChars));
						Document doc = progressArea.getDocument();
						progressArea.select(doc.getLength(), doc.getLength()); 
						len = newLen;
						f.repaint();
					}
					try {
						Thread.sleep(100);
					} catch (InterruptedException e) {}
				}
				
				if (job.exception != null) {
					ByteArrayOutputStream to =new ByteArrayOutputStream();
					PrintStream s = new PrintStream(to);
					job.exception.printStackTrace(s);
					try {
						to.close();
					} catch (IOException e) {
						e.printStackTrace();
					}
					progressArea.append(new String(to.toByteArray()));
					Document doc = progressArea.getDocument();
					progressArea.select(doc.getLength(), doc.getLength()); 
					return;
				}
				
				BufferedImage image;
				try {
					image = ImageIO.read(file);
					f.remove(progressScroller);
					imageLabel.setIcon(new ImageIcon(image));
					f.add(imageLabel);
					f.pack();
				} catch (IOException e) {
					e.printStackTrace();
				}
			}
		};
		progressJob.start();
	}
	
	private class RenderJob extends Thread {

		private File
			imageFile = null;
		private Writer
			stdoutWriter = null;
		private Exception
			exception = null;
		
		public RenderJob(File imageFile, Writer stdoutWriter) {
			super("Blender Renderer");
			this.imageFile = imageFile;
			this.stdoutWriter = stdoutWriter;
		}


		@Override
		public void run() {
			BlenderConnection c = new BlenderConnection();
			c.setStdoutRedirect(stdoutWriter);
			JrScene s = new JrScene(viewer.getSceneRoot());
			s.addPath("cameraPath", viewer.getCameraPath());
			try {
				c.renderImage(s, imageFile);
			} catch (IOException e) {
				//try setting the blender executable
				Runnable r = new Runnable() {
					@Override
					public void run() {
						Window w = SwingUtilities.getWindowAncestor(parentComp);
						showBlenderExcutableDialog(w);		
					}
				};
				try {
					EventQueue.invokeAndWait(r);
				} catch (Exception e2) {
					log.warning(e2.getMessage());
				}
				try {
					c.renderImage(s, imageFile);
				} catch (Exception e1) {
					exception = e1;
					JOptionPane.showMessageDialog(parentComp, "Could not write blender file", "Error", ERROR_MESSAGE);
				}
			}
		}
		
	}
	

	protected static void showBlenderExcutableDialog(Component parentComp) {
		Icon blenderIcon = new ImageIcon(ExportBlenderImage.class.getResource("blender.png"), "Blender Icon");
		String[] options = {"Choose Executable", "Cancel"};
		int result = JOptionPane.showOptionDialog(parentComp, "Blender executable not found.", "Export Error", YES_NO_CANCEL_OPTION, INFORMATION_MESSAGE, blenderIcon, options, null);
		if (result != 0) return;
		result = executableChooser.showOpenDialog(parentComp);
		if (result != JFileChooser.APPROVE_OPTION) return;
		File executable = executableChooser.getSelectedFile();
		if (executable.isDirectory()) { // mac application package
			executable = new File(executable + "/Contents/MacOS/blender");
		}
		BlenderConnection.setBlenderExecutable(executable);
	}

}