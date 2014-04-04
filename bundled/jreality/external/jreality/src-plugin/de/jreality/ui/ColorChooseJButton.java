package de.jreality.ui;

import java.awt.Color;
import java.awt.GradientPaint;
import java.awt.Graphics;
import java.awt.Graphics2D;
import java.awt.RenderingHints;
import java.awt.Window;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.util.LinkedList;

import javax.swing.JButton;
import javax.swing.JColorChooser;
import javax.swing.SwingUtilities;


/**
 * A color chooser button
 * <p>
 * Copyright 2005 <a href="http://www.sechel.de">Stefan Sechelmann</a>
 * <a href="http://www.math.tu-berlin.de/geometrie">TU-Berlin</a> 
 * @author Stefan Sechelmann
 */
@SuppressWarnings("serial")
public class ColorChooseJButton extends JButton implements ActionListener{

	private Color
		color = Color.WHITE;
	private LinkedList<ColorChangedListener>
		changeListeners = new LinkedList<ColorChangedListener>();
	private String 
		title = "Choose A Color";
	
	
	public static interface ColorChangedListener{
		public void colorChanged(ColorChangedEvent cce);
	}
	
	public static class ColorChangedEvent extends ActionEvent{
		private Color color = null;
		public ColorChangedEvent(Object source, Color color){
			super(source, 0, "color changed");
			this.color = color;
		}
		public Color getColor() {
			return color;
		}
	}
	
	public ColorChooseJButton() {
		this(true);
	}
	
	
	public ColorChooseJButton(boolean showDialogOnAction) {
		if (showDialogOnAction) {
			super.addActionListener(this);
		}
	}
	
	public ColorChooseJButton(Color color, boolean showDialogOnAction){
		this(showDialogOnAction);
		this.color = color;
	}
	
	
	public void addColorChangedListener(ColorChangedListener l){
		changeListeners.add(l);
	}
	
	public void removeColorChangedListener(ColorChangedListener l){
		changeListeners.remove(l);
	}
	
	public void removeAllColorChangedListeners(){
		changeListeners.clear();
	}
	
	protected void fireColorChanged(Color color){
		for (ColorChangedListener c : changeListeners) {
			c.colorChanged(new ColorChangedEvent(this, color));
		}
	}


	@Override
	public void paint(Graphics g) {
		super.paint(g);
		Graphics2D g2d = (Graphics2D)g;
		Color actColor = isEnabled() ? color : Color.LIGHT_GRAY;
		g2d.setRenderingHint(RenderingHints.KEY_TEXT_ANTIALIASING, RenderingHints.VALUE_TEXT_ANTIALIAS_ON);
		g2d.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_ON);
		g2d.setPaint(new GradientPaint(0, 0, getBackground(), 0, getHeight(), actColor));
		g2d.fillRoundRect(5, 3, getWidth() - 12, getHeight() - 8, 7, 7);
		g2d.setColor(actColor.darker());
		g2d.drawRoundRect(5, 3, getWidth() - 12, getHeight() - 8, 7, 7);
	}


	public void actionPerformed(ActionEvent e) {
		Window w = SwingUtilities.getWindowAncestor(this); 
		Color newColor = JColorChooser.showDialog(w, title, color);
		if (newColor != null){
			color = newColor;
			fireColorChanged(newColor);
		}
		repaint();
	}
	
	public void setTitle(String title) {
		this.title = title;
	}
	
	public void setColor(Color color) {
		this.color = color;
		repaint();
	}
	
	public Color getColor() {
		return color;
	}
	
}
