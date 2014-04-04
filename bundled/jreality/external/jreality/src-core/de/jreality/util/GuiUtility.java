package de.jreality.util;

import java.awt.Color;
import java.awt.Component;
import java.awt.Cursor;
import java.awt.Graphics2D;
import java.awt.Point;
import java.awt.image.BufferedImage;

/**
 * Static methods for hiding and showing the cursor.
 * @author weissmann
 *
 */
public class GuiUtility {

	public static void hideCursor(Component frame) {
//		 disable mouse cursor in fullscreen mode
	    BufferedImage cursorImg = new BufferedImage(16, 16, BufferedImage.TYPE_INT_ARGB);
	    Graphics2D gfx = cursorImg.createGraphics();
	    gfx.setColor(new Color(0, 0, 0, 0));
	    gfx.fillRect(0, 0, 16, 16);
	    gfx.dispose();
	    frame.setCursor(frame.getToolkit().createCustomCursor(cursorImg, new Point(), ""));
	}
	
	public static void showCursor(Component frame) {
	    frame.setCursor(Cursor.getDefaultCursor());
	}
	
}
