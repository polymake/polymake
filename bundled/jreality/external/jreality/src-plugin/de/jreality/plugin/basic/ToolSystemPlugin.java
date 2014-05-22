package de.jreality.plugin.basic;


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

import java.io.IOException;
import java.lang.reflect.Constructor;
import java.net.URL;
import java.security.PrivilegedAction;

import javax.swing.event.ChangeEvent;
import javax.swing.event.ChangeListener;

import de.jreality.plugin.icon.ImageHook;
import de.jreality.toolsystem.ToolSystem;
import de.jreality.toolsystem.config.ToolSystemConfiguration;
import de.jreality.ui.viewerapp.ViewerSwitch;
import de.jreality.util.Input;
import de.jreality.util.RenderTrigger;
import de.jreality.util.Secure;
import de.jreality.util.SystemProperties;
import de.jtem.jrworkspace.plugin.Controller;
import de.jtem.jrworkspace.plugin.Plugin;
import de.jtem.jrworkspace.plugin.PluginInfo;

/**
 * @author Steffen Weissmann
 */
public class ToolSystemPlugin extends Plugin implements ChangeListener {

	private ToolSystem toolSystem;
	private boolean synchRender = false;

	private ToolSystemConfiguration toolSystemConfiguration;
	private RunningEnvironment runningEnvironment;
	private String toolConfig;

	void init(View view) {
		// determine running environment
		String environment = Secure.getProperty(SystemProperties.ENVIRONMENT, SystemProperties.ENVIRONMENT_DEFAULT);
		if ("portal".equals(environment)) {
			runningEnvironment = RunningEnvironment.PORTAL; 
		} else if ("portal-remote".equals(environment)) {
			runningEnvironment = RunningEnvironment.PORTAL_REMOTE;
		} else {
			runningEnvironment = RunningEnvironment.DESKTOP;
		}
		toolConfig = Secure.getProperty(SystemProperties.TOOL_CONFIG, SystemProperties.TOOL_CONFIG_DEFAULT);
		// retrieve autoRender & synchRender system properties
		String synchRenderProp = Secure.getProperty(SystemProperties.SYNCH_RENDER, SystemProperties.SYNCH_RENDER_DEFAULT);
		if (synchRenderProp.equalsIgnoreCase("true")) {
			synchRender = true;
		}
		ViewerSwitch viewerSwitch = view.getViewer();
		RenderTrigger trigger = view.getRenderTrigger();
		if (trigger != null && synchRender) {
			trigger.setAsync(false);
		}
		toolSystem = createToolSystem(viewerSwitch, synchRender ? trigger : null);
		
	}
	
	private ToolSystem createToolSystem(final ViewerSwitch viewerSwitch, final RenderTrigger synchRenderTrigger) {
		return Secure.doPrivileged(new PrivilegedAction<ToolSystem>() {
			public ToolSystem run() {
				
				// load tool system configuration
				toolSystemConfiguration = Secure.doPrivileged(new PrivilegedAction<ToolSystemConfiguration>() {
					public ToolSystemConfiguration run() {
						
						ToolSystemConfiguration cfg=null;
						// HACK: only works for "regular" URLs
						try {
							if (toolConfig.contains("://")) {
								cfg = ToolSystemConfiguration.loadConfiguration(new Input(new URL(toolConfig)));
							} else {
								if (toolConfig.equals("default")) cfg = ToolSystemConfiguration.loadDefaultDesktopConfiguration();
								if (toolConfig.equals("portal")) cfg = ToolSystemConfiguration.loadDefaultPortalConfiguration();
								if (toolConfig.equals("portal-remote")) cfg = ToolSystemConfiguration.loadRemotePortalConfiguration();
								if (toolConfig.equals("default+portal")) cfg = ToolSystemConfiguration.loadDefaultDesktopAndPortalConfiguration();
							}
						} catch (IOException e) {
							// should not happen
							e.printStackTrace();
						}
						if (cfg == null) throw new IllegalStateException("couldn't load config ["+toolConfig+"]");
						return cfg;
					}
				});
				
				ToolSystem ts = null;
				try {
					if (runningEnvironment == RunningEnvironment.PORTAL_REMOTE) {
						try {
							Class<?> clazz = Class.forName("de.jreality.toolsystem.PortalToolSystemImpl");
							Class<? extends ToolSystem> portalToolSystem = clazz.asSubclass(ToolSystem.class);
							Constructor<? extends ToolSystem> cc = portalToolSystem.getConstructor(new Class[]{de.jreality.jogl.JOGLViewer.class, ToolSystemConfiguration.class});
							de.jreality.jogl.JOGLViewer cv = (de.jreality.jogl.JOGLViewer) viewerSwitch.getCurrentViewer();
							ts = cc.newInstance(new Object[]{cv, toolSystemConfiguration});
						} catch (Throwable t) {
							t.printStackTrace();
						}
					} else {
						ts = new ToolSystem(
								viewerSwitch,
								toolSystemConfiguration,
								synchRenderTrigger
						);
						viewerSwitch.setToolSystem(ts);
					}
				} catch (Exception e) {
					e.printStackTrace();
				}
				return ts;
			}
		});
	}
	
	public void stateChanged(ChangeEvent e) {
		if (e.getSource() instanceof Scene) {
			Scene scene = (Scene) e.getSource();
			updateScenePaths(scene);
		}
	}

	private void updateScenePaths(Scene scene) {
		toolSystem.setEmptyPickPath(scene.getEmptyPickPath());
		toolSystem.setAvatarPath(scene.getAvatarPath());
	}

	@Override
	public PluginInfo getPluginInfo() {
		PluginInfo info = new PluginInfo();
		info.name = "Tool system";
		info.vendorName = "Steffen Weissmann"; 
		info.icon = ImageHook.getIcon("hausgruen.png");
		info.isDynamic = false;
		return info;
	}

	@Override
	public void install(Controller c) throws Exception {
		Scene scene = c.getPlugin(Scene.class);
		View view = c.getPlugin(View.class);
		init(view);
		updateScenePaths(scene);
		toolSystem.initializeSceneTools();
		scene.addChangeListener(this);
	}

	@Override
	public void uninstall(Controller c) throws Exception {
		Scene scene = c.getPlugin(Scene.class);
		scene.removeChangeListener(this);
		toolSystem.dispose();
	}

	public ToolSystem getToolSystem() {
		return toolSystem;
	}
	
}