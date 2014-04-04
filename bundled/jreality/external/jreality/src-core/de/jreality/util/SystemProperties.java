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


package de.jreality.util;

import java.net.InetAddress;
import java.net.UnknownHostException;


/**
 * jReality system property keys and default values
 * used in methods of {@link Secure}.
 * 
 * @author msommer
 */
public class SystemProperties {

	private SystemProperties() {}
	
	public static String hostname="localhost";
	public static boolean isPortal = false;
	static {
		try {
			hostname = InetAddress.getLocalHost().getHostName();
		} catch (UnknownHostException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		String foo = Secure.getProperty(SystemProperties.ENVIRONMENT);
		if (foo != null && foo.indexOf("portal") != -1) isPortal = true;
	}
	
	/** 
	 * Specifies whether {@link de.jreality.ui.viewerapp.ViewerApp} initializes a {@link RenderTrigger}
	 * for the displayed scene.<br> 
	 * Values: <code>true | false</code>
	 * @see SystemProperties#SYNCH_RENDER   
	 */
	public final static String AUTO_RENDER = "de.jreality.ui.viewerapp.autoRender";
	public final static String AUTO_RENDER_DEFAULT = "true";
	
	/** 
	 * Specifies whether the {@link RenderTrigger} initialized by {@link de.jreality.ui.viewerapp.ViewerApp}
	 * dispatches synchronous render requests.<br>
	 * Values: <code>true | false</code>
	 * @see SystemProperties#AUTO_RENDER
	 */
	public final static String SYNCH_RENDER = "de.jreality.ui.viewerapp.synchRender";
	public final static String SYNCH_RENDER_DEFAULT = "true";

	/** 
	 * Specifies the default JrScene used by {@link de.jreality.ui.viewerapp.ViewerApp}.<br>
	 * Values: <code>desktop | portal | portal-remote</code>
	 */
	public final static String ENVIRONMENT = "de.jreality.viewerapp.env";
	public final static String ENVIRONMENT_DEFAULT = "desktop";
	
	/** 
	 * Specifies the {@link de.jreality.toolsystem.config.ToolSystemConfiguration} to be used by a {@link de.jreality.toolsystem.ToolSystem}.<br>
	 * Values: <code>default | portal | portal-remote | desfault+portal</code>
	 * @see SystemProperties#TOOL_CONFIG_FILE 
	 */
	public final static String TOOL_CONFIG = "de.jreality.scene.tool.Config";
	public final static String TOOL_CONFIG_DEFAULT = "default";
	
	/**
	 * Specifies the {@link de.jreality.toolsystem.config.ToolSystemConfiguration} to be used by a {@link de.jreality.toolsystem.ToolSystem}.<br>
	 * Values: file name of a tool system cofiguration xml-file
	 * @see SystemProperties#TOOL_CONFIG
	 */
	public final static String TOOL_CONFIG_FILE = "jreality.toolconfig";
	
	/**
	 * Specifies the viewer(s) to be initialized by {@link de.jreality.ui.viewerapp.ViewerApp}.<br>
	 * Values: class names of {@link de.jreality.scene.Viewer} implementations separated by space character 
	 */
	public final static String VIEWER = de.jreality.scene.Viewer.class.getName();
	public final static String VIEWER_DEFAULT_JOGL = "de.jreality.jogl.JOGLViewer";  //de.jreality.jogl.JOGLViewer.class.getName();
	public final static String VIEWER_DEFAULT_JOGL3 = "de.jreality.jogl3.JOGL3Viewer";

	public final static String VIEWER_JOGL_DOME = "de.jreality.jogl.DomeViewer";
	public final static String VIEWER_JOGL3_DOME = "de.jreality.jogl3.JOGL3DomeViewer";
	
	public final static String VIEWER_DEFAULT_SOFT = "de.jreality.softviewer.SoftViewer";  //de.jreality.softviewer.SoftViewer.class.getName();
//	public final static String VIEWER_DEFAULT_PORTAL = "de.jreality.portal.PortalServerViewer";  //de.jreality.portal.PortalServerViewer.class.getName();
	public final static String CROSS_EYED_STEREO = "jreality.crossEyedStereo"; // set to false to get wall-eyed
	public final static String CROSS_EYED_STEREO_DEFAULT = "true"; // set to false to get wall-eyed
	/**
	 * Specifies the path of the jReality data directory.<br>
	 * Values: directory path
	 */
	public final static String JREALITY_DATA = "jreality.data";
	public final static String JREALITY_DATA_DEFAULT="/net/MathVis/data/testData3D";
	
	
	/**
	 * Maximal number of polygon vertices in {@link de.jreality.soft.Polygon}.<br>
	 * Values: non-negative integer
	 */
	public final static String SOFT_MAX_POLYVERTEX = "jreality.soft.maxpolyvertex";
	
	/**
	 * Specifies the {@link de.jreality.soft.Imager} used in {@link de.jreality.soft.NewPolygonRasterizer}.<br>
	 * Values: <code>hatch | toon</code> (instances of {@link de.jreality.soft.Imager})
	 */
	public final static String SOFT_IMAGER = "jreality.soft.imager";
	
	
	
	/**
	 * Flag converted into static field by {@link de.jreality.jogl.JOGLConfiguration}.<br>
	 * Values: <code>true | false</code>
	 */
	public final static String JOGL_DEBUG_GL = "jreality.jogl.debugGL";
	
	/**
	 * Flag converted into static field by {@link de.jreality.jogl.JOGLConfiguration}.<br>
	 * Values: <code>true | false</code>
	 */
	public final static String JOGL_QUAD_BUFFERED_STEREO = "jreality.jogl.quadBufferedStereo";
	public final static String JOGL_LEFT_STEREO = "jreality.jogl.leftStereo";
    public final static String JOGL_RIGHT_STEREO = "jreality.jogl.rightStereo";
    public final static String JOGL_MASTER_STEREO = "jreality.jogl.masterStereo";

	/**
	 * Flag converted into static field by {@link de.jreality.jogl.JOGLConfiguration}.<br>
	 * Values: <code>true | false</code>
	 */
	public final static String JOGL_COPY_CAT = "discreteGroup.copycat";

	/**
	 * Flag converted into static field by {@link de.jreality.jogl.JOGLConfiguration}.<br>
	 * Values: <code>true | false</code>
	 */
	public final static String JOGL_BLEND_FUNC_SEPARATE = "jogl.hasBlendFuncSeparate";

	/**
	 * Flag converted into static field by {@link de.jreality.jogl.JOGLConfiguration}.<br>
	 * Values: <code>finest | finer | fine | info</code> (static fields of {@link java.util.logging.Level})
	 */
	public final static String JOGL_LOGGING_LEVEL = "jreality.jogl.loggingLevel";

	
	/**
	 * Specifies a properties file used in {@link de.jreality.util.ConfigurationAttributes} 
	 * for managing configuration settings.<br>
	 * Values: file path
	 */
	public final static String CONFIG_SETTINGS = "jreality.config";
	public final static String CONFIG_SETTINGS_DEFAULT = "jreality.props";  //in current directory 

	/**
	 * Specifies the selection manager to be used.<br>
	 * Values: instances of {@link de.jreality.ui.viewerapp.SelectionManager}
	 */
	public final static String SELECTION_MANAGER = "de.jreality.ui.viewerapp.SelectionManager";
	public final static String SELECTION_MANAGER_DEFAULT = "de.jreality.ui.viewerapp.SelectionManagerImpl";
	
	/**
	 * Specifies the viewer to be initialized by {@link de.jreality.portal.HeadTrackedViewer#HeadTrackedViewer()}.<br>
	 * Values: class name of {@link de.jreality.scene.Viewer} implementation
	 */
	public final static String PORTAL_HEADTRACKED_VIEWER = "de.jreality.portal.HeadTrackedViewer";
	public final static String PORTAL_HEADTRACKED_VIEWER_DEFAULT = "de.jreality.jogl.JOGLViewer";

	/**
	 * Specifies the location of <code>bsh.jar</code> for reading bsh script files.
	 * Values: file path
	 * @see de.jreality.reader.ReaderBSH
	 */
	public final static String BSH_JAR = "jreality.bsh.jar";
	
	
	/**
	 * Specifies the scale for the portal coordinate system.
	 * Values: double
	 * @see de.jreality.portal.PortalCoordinateSystem
	 */
	public final static String PORTAL_SCALE = "portalScale";
	public final static String PORTAL_SCALE_DEFAULT = "1.0";

}
