package de.jreality.plugin.icon;

import static java.awt.Image.SCALE_SMOOTH;
import static java.awt.image.BufferedImage.TYPE_INT_ARGB;

import java.awt.Graphics2D;
import java.awt.Image;
import java.awt.image.BufferedImage;
import java.io.IOException;
import java.io.InputStream;
import java.util.logging.Logger;

import javax.imageio.ImageIO;
import javax.swing.Icon;
import javax.swing.ImageIcon;

import de.jreality.util.LoggingSystem;

/**
 * An image loader for jar files
 * <p>
 * Copyright 2005 <a href="http://www.sechel.de">Stefan Sechelmann</a>
 * <a href="http://www.math.tu-berlin.de/geometrie">TU-Berlin</a> 
 * @author Stefan Sechelmann
 */
public class ImageHook { 


	private static Logger
		log = LoggingSystem.getLogger(ImageHook.class);
	
	public static Image getImage(String filename){ 
		InputStream in = ImageHook.class.getResourceAsStream(filename);
		if (in == null)
			return null;
		Image result = null;
		try {
			result = ImageIO.read(in);
		} catch (IOException e) {
			log.warning(e.getLocalizedMessage());
		} finally {
			try {
				in.close();
			} catch (IOException e) {
				e.printStackTrace();
			}
		}
		return result;
	}

	public static Image getImage(String filename, int width, int height) {
		Image image = getImage(filename);
		if (image == null) {
			return null;
		} else {
			return image.getScaledInstance(width, height, SCALE_SMOOTH);
		}
	}
	
	
	public static Icon getIcon(String filename) {
		Image image = getImage(filename);
		if (image == null) {
			return null;
		} else {
			return new ImageIcon(image);	
		}
	}

	public static Icon getIcon(String filename, int width, int height) {
		Image image = getImage(filename, width, height);
		if (image == null) {
			return null;
		} else {
			return new ImageIcon(image);	
		}
	}
	
	
	public static Image renderIcon(Icon icon) {
		BufferedImage image = new BufferedImage(icon.getIconWidth(), icon.getIconWidth(), TYPE_INT_ARGB);
		Graphics2D g = image.createGraphics();
		icon.paintIcon(null, g, 0, 0);
		return image;
	}
	
	
	public static Icon scaleIcon(Icon icon, int width, int height) {
		Image imageNewSize = null;
		if (icon instanceof ImageIcon) {
			ImageIcon imageIcon = (ImageIcon)icon;
			imageNewSize = imageIcon.getImage().getScaledInstance(width, height, SCALE_SMOOTH);
		} else  {
			Image image = renderIcon(icon);
			imageNewSize = scaleImage(image, width, height);
		}
		return new ImageIcon(imageNewSize);
	}
	
	
	public static Image scaleImage(Image image, int width, int height) {
		return image.getScaledInstance(width, height, SCALE_SMOOTH);
	}
	
}

