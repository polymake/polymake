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

package de.jreality.ui.viewerapp;

import static de.jreality.util.SystemProperties.JREALITY_DATA;

import java.awt.Component;
import java.beans.PropertyChangeEvent;
import java.beans.PropertyChangeListener;
import java.io.File;

import javax.swing.JComponent;
import javax.swing.JFileChooser;
import javax.swing.JOptionPane;
import javax.swing.filechooser.FileSystemView;
import javax.swing.plaf.basic.BasicFileChooserUI;

import de.jreality.util.Secure;

public class FileLoaderDialog {

	static File lastDir = new File(Secure.getProperty(JREALITY_DATA, "/net/MathVis/data/testData3D"));

	public static void setLastDir(File dir) {
		lastDir = dir;
	}

	public static File getLastDir() {
		return lastDir;
	}

	public static JFileChooser createFileChooser() {
		return createFileChooser(true, FileFilter.createJRealityDataFilter());
	}

	public static JFileChooser createFileChooser(final String ext, final String description) {
		return createFileChooser(true, new FileFilter(description, ext));
	}

	public static JFileChooser createFileChooser(
		boolean useAcceptAllFileFilter,
		javax.swing.filechooser.FileFilter... ff
	) {
		FileSystemView view = FileSystemView.getFileSystemView();
		JFileChooser chooser = new JFileChooser(!lastDir.exists() ? view.getHomeDirectory() : lastDir, view);
		chooser.setAcceptAllFileFilterUsed(false);
		for (int i = 0; i < ff.length; i++) {
			chooser.addChoosableFileFilter(ff[i]);
		}
		if (useAcceptAllFileFilter) {
			javax.swing.filechooser.FileFilter aff = new AllFileFilterGtkBugWorkaround();
			chooser.addChoosableFileFilter(aff);
			chooser.setFileFilter(aff);
		}
		if (ff.length != 0) {
			chooser.setFileFilter(ff[0]);
		} 
		return chooser;
	}

	private static class AllFileFilterGtkBugWorkaround extends
			javax.swing.filechooser.FileFilter {

		public boolean accept(File file) {
			return true;
		}
		public String getDescription() {
			return "All Files";
		}
		@Override
		public String toString() {
			return getDescription();
		}

	}

	// -- OPEN FILE DIALOGS ----------------------------------

	private static File[] loadFiles(Component parent, JFileChooser chooser,
			JComponent accessory) {
		if (accessory != null)
			chooser.setAccessory(accessory);
		chooser.setMultiSelectionEnabled(true);
		if (chooser.showOpenDialog(parent) != JFileChooser.APPROVE_OPTION)
			return null;
		File[] files = chooser.getSelectedFiles();
		lastDir = chooser.getCurrentDirectory();
		return files;
	}

	private static File loadFile(Component parent, JFileChooser chooser,
			JComponent accessory) {
		if (accessory != null)
			chooser.setAccessory(accessory);
		chooser.setMultiSelectionEnabled(false);
		if (chooser.showOpenDialog(parent) != JFileChooser.APPROVE_OPTION)
			return null;
		File file = chooser.getSelectedFile();
		lastDir = chooser.getCurrentDirectory();
		return file;
	}

	public static File[] loadFiles(Component parent) {
		return loadFiles(parent, (JComponent) null);
	}

	public static File[] loadFiles(Component parent, JComponent accessory) {
		JFileChooser chooser = createFileChooser(); // adds default file filter
													// for jReality 3D data
													// files
		return loadFiles(parent, chooser, accessory);
	}

	public static File loadFile(Component parent, String extension,
			String description) {
		JFileChooser chooser = createFileChooser(extension, description);
		return loadFile(parent, chooser, (JComponent) null);
	}

	public static File loadFile(Component parent,
			boolean useAcceptAllFileFilter,
			javax.swing.filechooser.FileFilter... ff) {
		JFileChooser chooser = createFileChooser(useAcceptAllFileFilter, ff);
		return loadFile(parent, chooser, (JComponent) null);
	}

	public static File loadFile(Component parent, JComponent accessory) {
		JFileChooser chooser = createFileChooser(true);
		return loadFile(parent, chooser, accessory);
	}

	// -- SAVE FILE DIALOGS ----------------------------------

	private static File selectTargetFile(
		Component parent,
		final JFileChooser chooser, 
		JComponent accessory
	) {
		if (accessory != null) {
			chooser.setAccessory(accessory);
		}
		chooser.setMultiSelectionEnabled(false);

		// don't clear filename text field when changing the filter
		chooser.addPropertyChangeListener(
				JFileChooser.FILE_FILTER_CHANGED_PROPERTY,
				new PropertyChangeListener() {
					public void propertyChange(PropertyChangeEvent e) {
						try {
							BasicFileChooserUI ui = (BasicFileChooserUI) chooser.getUI();
							chooser.setSelectedFile(new File(ui.getFileName()));
							chooser.updateUI();
						} catch (Exception exc) { }
					}
				});

		// get target file and let user confirm an overwriting
		File file = null;
		while (true) {
			int choice = chooser.showSaveDialog(parent);
			if (choice == JFileChooser.APPROVE_OPTION) {
				File chosen = chooser.getSelectedFile();
				try { // append preferred extension of used file filter if
						// existing and user did not specify one
					FileFilter filter = (FileFilter) chooser.getFileFilter();
					if (!filter.accept(chosen)) { // invalid extension
						String extension = filter.getPreferredExtension();
						if (extension != null)
							chosen = new File(chosen.getPath() + "."
									+ extension);
					}
				} catch (Exception e) { }

				if (!chosen.exists()) {
					file = chosen;
					break;
				} else { // file exists
					int confirm = JOptionPane.showConfirmDialog(parent,
						"Overwrite file " + chosen.getName() + "?"
					);
					if (confirm == JOptionPane.OK_OPTION) {
						file = chosen;
						break;
					} else if (confirm == JOptionPane.NO_OPTION) {
						continue;
					}
					break; // confirm == CANCEL_OPTION
				}
			} else {
				break; // choice == CANCEL_OPTION
			}
		}
		lastDir = chooser.getCurrentDirectory();

		return file;
	}

	public static File selectTargetFile(Component parent) {
		return selectTargetFile(parent, (JComponent) null);
	}

	public static File selectTargetFile(Component parent, JComponent accessory) {
		JFileChooser chooser = createFileChooser();
		return selectTargetFile(parent, chooser, accessory);
	}

	public static File selectTargetFile(Component parent, String extension,
			String description) {
		return selectTargetFile(parent, (JComponent) null, extension,
				description);
	}

	public static File selectTargetFile(Component parent, JComponent accessory,
			String extension, String description) {
		JFileChooser chooser = createFileChooser(extension, description);
		return selectTargetFile(parent, chooser, accessory);
	}

	public static File selectTargetFile(Component parent,
			boolean useAcceptAllFileFilter,
			javax.swing.filechooser.FileFilter... ff) {
		return selectTargetFile(parent, (JComponent) null,
				useAcceptAllFileFilter, ff);
	}

	public static File selectTargetFile(Component parent, JComponent accessory,
			boolean useAcceptAllFileFilter,
			javax.swing.filechooser.FileFilter... ff) {
		JFileChooser chooser = createFileChooser(useAcceptAllFileFilter, ff);
		return selectTargetFile(parent, chooser, accessory);
	}

}