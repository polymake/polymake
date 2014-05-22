package de.jreality.sunflow;

import java.awt.Dimension;
import java.awt.Toolkit;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;

import javax.swing.JFrame;
import javax.swing.JOptionPane;

import org.sunflow.SunflowAPI;
import org.sunflow.core.Display;
import org.sunflow.core.display.FileDisplay;
import org.sunflow.core.display.OpenExrDisplay;
import org.sunflow.image.Color;
import org.sunflow.system.UI;

public class RenderDisplay implements Display {
	private JFrame frame;
	private CancelableImagePanel imagePanel;
	private Display fileDisplay;

	public RenderDisplay() {
		this(null);
	}

	public RenderDisplay(String filename) {
		if (filename != null) {
			if (filename.endsWith(".exr")) {
				fileDisplay = new OpenExrDisplay(filename, null, null);
			} else {
				fileDisplay = new FileDisplay(filename);
			}
		}
		frame = null;
	}

	public void imageBegin(int w, int h, int bucketSize) {
		if (fileDisplay != null) {
			fileDisplay.imageBegin(w, h, bucketSize);
		}
		if (frame == null) {
			frame = new JFrame("Sunflow v" + SunflowAPI.VERSION);
			frame.addWindowListener(new WindowAdapter() {
				@Override
				public void windowClosing(WindowEvent e) {
					if (!imagePanel.isDone()) {
						if (JOptionPane.showConfirmDialog(
								frame,
								"Really abort all ongoing rendering jobs?",
								"Sunflow",
								JOptionPane.YES_NO_OPTION
						) == JOptionPane.YES_OPTION) {
							UI.taskCancel();
						}
					}
				}
			});
			imagePanel = new CancelableImagePanel();
			imagePanel.imageBegin(w, h, bucketSize);
			Dimension screenRes = Toolkit.getDefaultToolkit().getScreenSize();
			boolean needFit = false;
			double width = w;
			double height = h;
			if (w >= (screenRes.getWidth() - 200)) {
				width = screenRes.getWidth() - 200;
				height *= width/w;
				needFit = true;
			}
			if (height >= (screenRes.getHeight() - 200)) {
				double newHeight = screenRes.getHeight() - 200;
				width *= newHeight/height;
				height = newHeight;
				needFit = true;
			}
			imagePanel.setPreferredSize(new Dimension((int) width, (int) height));
				
			System.out.println("dimension "+w+", "+h);
			frame.setContentPane(imagePanel);
			frame.pack();
			frame.setLocationRelativeTo(null);
			frame.setVisible(true);
			if (needFit) imagePanel.fit();
			imagePanel.imageBegin(w, h, bucketSize);
		}
	}

	public void imagePrepare(int x, int y, int w, int h, int id) {
		if (fileDisplay != null) {
			fileDisplay.imagePrepare(x, y, w, h, id);
		}
		imagePanel.imagePrepare(x, y, w, h, id);
	}

	public void imageUpdate(int x, int y, int w, int h, Color[] data) {
		if (fileDisplay != null) {
			fileDisplay.imageUpdate(x, y, w, h, data);
		}
		imagePanel.imageUpdate(x, y, w, h, data);
	}

	public void imageFill(int x, int y, int w, int h, Color c) {
		if (fileDisplay != null) {
			fileDisplay.imageFill(x, y, w, h, c);
		}
		imagePanel.imageFill(x, y, w, h, c);
	}

	public void imageEnd() {
		if (fileDisplay != null) {
			fileDisplay.imageEnd();
		}
		imagePanel.imageEnd();
	}

	public JFrame getFrame() {
		return frame;
	}
}
