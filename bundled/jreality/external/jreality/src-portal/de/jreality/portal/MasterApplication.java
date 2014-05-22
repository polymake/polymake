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

import java.io.IOException;
import java.net.ServerSocket;

import de.jreality.scene.proxy.smrj.ClientFactory;
import de.jreality.scene.tool.InputSlot;
import de.jreality.toolsystem.DeviceManager;
import de.jreality.toolsystem.PortalToolSystem;
import de.jreality.toolsystem.ToolEvent;
import de.jreality.toolsystem.ToolEventQueue;
import de.jreality.toolsystem.ToolEventReceiver;
import de.jreality.toolsystem.config.ToolSystemConfiguration;
import de.smrj.Broadcaster;
import de.smrj.tcp.TCPBroadcasterNIO;
import de.smrj.tcp.management.JarServer;
import de.smrj.tcp.management.Local;

/**
 * Starts a ToolSystem with Trackd device and a remote application on the different walls of the portal.
 * Default is to start ViewerVR, but you can give another classname. The given class needs at least one
 * static method called remoteMain:
 * 
 * public static ViewerApp remoteMain(String[] args) { ... }
 * 
 * 
 * 
 * @author weissman
 *
 */
public class MasterApplication {
	
	protected static final InputSlot SYSTEM_TIME = InputSlot.getDevice("SystemTime");
	DeviceManager deviceManager;
	ToolEventQueue eventQueue;
	
	static final boolean DUMP_RENDER=false, DUMP_SEND = false;
	
	public MasterApplication(final PortalToolSystem receiver) throws IOException {
		ToolSystemConfiguration config = ToolSystemConfiguration.loadRemotePortalMasterConfiguration();
		eventQueue = new ToolEventQueue(new ToolEventReceiver() {
			long st;
			public void processToolEvent(ToolEvent event) {
				if (DUMP_SEND) st = -System.currentTimeMillis();
				receiver.processToolEvent(event);
				if (DUMP_SEND) {
					st+=System.currentTimeMillis();
					System.out.println("send took "+st+" ms: "+event.getInputSlot());
				}

				if (event.getInputSlot() == SYSTEM_TIME) {
					
					
					if (DUMP_RENDER) st = -System.currentTimeMillis();
					receiver.render();
					if (DUMP_RENDER) {
						st+=System.currentTimeMillis();
						System.out.println("render took "+st+" ms");
					}
					
//					st = -System.currentTimeMillis();
//					receiver.swapBuffers();
//					st+=System.currentTimeMillis();
//					System.out.println("swapBuffers took "+st+" ms");
					
					// without this sleep the shared memory from trackd is blocked...
					// (Trackd can then no longer write any updates.)
					try {
						Thread.sleep(1);
					} catch (InterruptedException e) {
						// TODO Auto-generated catch block
						e.printStackTrace();
					}

				}
			}
		});
		deviceManager = new DeviceManager(config, eventQueue, null);
		eventQueue.start();
	}

	public static void main(String[] args) throws IOException, ClassNotFoundException {
		int port = 8844;
		int cpPort = 8845;
		JarServer js = new JarServer(new ServerSocket(cpPort));
		Broadcaster bc = new TCPBroadcasterNIO(port, Broadcaster.RESPONSE_TYPE_EXCEPTION);
		Local.sendStart(port, cpPort, Broadcaster.RESPONSE_TYPE_EXCEPTION, ClientFactory.class);
		js.waitForDownloads();
		//class is either the 0-th argument or ViewerVR
		Class appClass = Class.forName(args.length == 0 ? "de.jreality.vr.ViewerVR" : args[0]);
		//other arguments are written in rest
		String[] rest = null;
		if (args.length > 1) rest = new String[args.length-1];
		if (rest != null) for (int i = 0; i<args.length-1; ++i) rest[i] = args[i+1];
		PortalToolSystem tr = bc.getRemoteFactory().createRemoteViaStaticMethod(
				PortalToolSystem.class, RemoteExecutor.class,
				"startRemote", new Class[]{Class.class, String[].class}, new Object[]{appClass, rest}); 
		new MasterApplication(tr);
	}
}
