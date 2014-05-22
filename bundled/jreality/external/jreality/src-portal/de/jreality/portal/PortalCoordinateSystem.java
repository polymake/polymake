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


package de.jreality.portal;

import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.geom.Rectangle2D;
import java.util.Vector;

import de.jreality.math.MatrixBuilder;
import de.jreality.math.Rn;
import de.jreality.scene.Camera;
import de.jreality.util.ConfigurationAttributes;
import de.jreality.util.Secure;
import de.jreality.util.SystemProperties;

public class PortalCoordinateSystem {

	/*
	 * comment from arMotionstarDriverPORTAL.cpp:
	 *   x -= 4.068; // move x=0 to the middle
	 * 	 y -= 1.36;  // move y=0 to the bottom of the visible screen (instead of the bottom)
	 * 	 z -= 4.068; // move z=0 to the center of the floor
	 */
	//  static double xDimPORTAL = 4.068;   // half PORTAL screen x-dim in feet
	//  static double yDimPORTAL = 6.561;   // full PORTAL screen y-dim in feet
	public static double xDimPORTAL = 2.48;   // full PORTAL screen x-dim in METER
	public static double yDimPORTAL = 2.00;   // full PORTAL screen y-dim in METER
	public static double zOffsetPORTAL = getxDimPORTAL()/2.0;
	public static double xOffsetPORTAL = -getxDimPORTAL()/2.0;
	public static double yOffsetPORTAL = 0.4;		// the height (in meters) of the base of the walls

	private static double portalScale = 1.0;
	static double[] portalCenter = {0,0,0,1};
	static double[] metersToPortal = Rn.identityMatrix(4);
	static double[] portalToMeters = Rn.identityMatrix(4);

	private static boolean changed = true;
	private static double eyeSeparationMeters = .07;
	private static double eyeLevelMeters = 1.7;

	static {
		String bar = Secure.getProperty(SystemProperties.PORTAL_SCALE);
		if (bar != null) setPortalScale(Double.parseDouble(bar));
//		System.err.println("PCS: Portal scale is "+getPortalScale());
		
		// read screen setup from config file:
		ConfigurationAttributes config = ConfigurationAttributes.getDefaultConfiguration();
		setxDimPORTAL(config.getDouble("screen.width", getxDimPORTAL()));
		yDimPORTAL = config.getDouble("screen.height", yDimPORTAL);

		xOffsetPORTAL = config.getDouble("screen.offset.x", xOffsetPORTAL);
		yOffsetPORTAL = config.getDouble("screen.offset.y", yOffsetPORTAL);

		zOffsetPORTAL = config.getDouble("screen.offset.z", zOffsetPORTAL);
		
	}
	
	public static double[] getPortalCenter() {
		return portalCenter;
	}

	public static void setPortalCenter(double[] portalCenter) {
		PortalCoordinateSystem.portalCenter = portalCenter;
		changed = true; update();
	}

	private static void update()	{
		if (!changed) return;
		MatrixBuilder.euclidean().scale(getPortalScale()).translate(portalCenter).assignTo(metersToPortal);
		Rn.inverse(portalToMeters, metersToPortal);
		changed = false;
	}

	public static double[] getMetersToPortal() {
		return metersToPortal;
	}

	public static double[] getPortalToMeters() {
		return portalToMeters;
	}

	public static Rectangle2D getWallPort() {
		Rectangle2D wp = new Rectangle2D.Double();
		wp.setFrame(
				portalScale*(xOffsetPORTAL-portalCenter[0]),
				portalScale*(yOffsetPORTAL-portalCenter[1]),
				portalScale*getxDimPORTAL(),
				portalScale*yDimPORTAL);
		return wp;
	}

	public static void setPORTALViewport(double[] origin, Camera cam) {

		double xmin=0, ymin=0;
		Rectangle2D wallport = getWallPort();
		double z = -origin[2] + convertMeters(zOffsetPORTAL);  // make wall z=0
		
		cam.setFocus(z);
		xmin = (wallport.getMinX()+(origin[0]))/z;
		ymin = (wallport.getMinY()+(origin[1]))/z;
		cam.setViewPort(new Rectangle2D.Double(xmin, ymin, wallport.getWidth()/z, wallport.getHeight()/z));
//		LoggingSystem.getLogger(CameraUtility.class).info("Setting camera viewport to "+cam.getViewPort().toString());
	}
	public static double convertMeters(double d) {
		return portalScale*d;
	}

	public static void setPortalScale(double portalScale) {
		PortalCoordinateSystem.portalScale = portalScale;
		broadcastChange();
	}

	public static double getPortalScale() {
		return portalScale;
	}

	public static void setEyeSeparationMeters(double eyeSeparation) {
		PortalCoordinateSystem.eyeSeparationMeters = eyeSeparation;
	}

	public static double getEyeSeparationMeters() {
		return eyeSeparationMeters;
	}

	public static void setEyeLevelMeters(double eyeLevel) {
		PortalCoordinateSystem.eyeLevelMeters = eyeLevel;
	}

	public static double getEyeLevelMeters() {
		return eyeLevelMeters;
	}

	static Vector<ActionListener> listeners = new Vector<ActionListener>();


	public static void addChangeListener(ActionListener l)	{
		if (listeners.contains(l)) return;
		listeners.add(l);
	}

	public static void removeChangeListener(ActionListener l)	{
		listeners.remove(l);
	}

	static PortalCoordinateSystem pcs = new PortalCoordinateSystem();

	public static void broadcastChange()	{
		if (listeners == null) return;
		ActionEvent e = new ActionEvent(pcs,0,null);
		//SyJOGLConfiguration.theLog.log(Level.INFO,"SelectionManager: broadcasting"+listeners.size()+" listeners");
		if (!listeners.isEmpty())	{
			//JOGLConfiguration.theLog.log(Level.INFO,"SelectionManager: broadcasting"+listeners.size()+" listeners");
			for (ActionListener l : listeners)	{
				l.actionPerformed(e);
			}
		}
	}

	public static double getxDimPORTAL() {
		return xDimPORTAL;
	}

	public static void setxDimPORTAL(double xDimPORTAL) {
		PortalCoordinateSystem.xDimPORTAL = xDimPORTAL;
	}

}
