package de.jreality.toolsystem.raw;

import java.io.IOException;
import java.net.SocketAddress;
import java.util.HashMap;
import java.util.Map;

import de.jreality.math.Matrix;
import de.jreality.scene.Viewer;
import de.jreality.scene.data.DoubleArray;
import de.jreality.scene.tool.AxisState;
import de.jreality.scene.tool.InputSlot;
import de.jreality.toolsystem.ToolEvent;
import de.jreality.toolsystem.ToolEventQueue;
import de.jreality.toolsystem.util.OSCPool;
import de.sciss.net.OSCListener;
import de.sciss.net.OSCMessage;
import de.sciss.net.OSCServer;


/**
 * Receives WiiMote events via OSC
 * 
 * Make sure to uncomment the WiiMoteOSC bits in toolconfig.xml if you want to use this driver.
 * 
 * The OSC detour is a bit of a kludge, but unfortunately none of the Java WiiMote libraries were
 * satisfactory, and the Bluetooth libraries for Java didn't seem to work well with WiiMotes, either.
 * 
 * 
 * Instructions for Mac OS X:
 *    - Install DarwiinRemoteOSC (http://mac.softpedia.com/get/Utilities/darwiinremoteOSC.shtml).
 *    - Launch DarwiinRemoteOSC and pair your WiiMote with the computer (i.e., push 1 and 2 on the WiiMote
 *      at the same time).  After a few seconds, you should see some squiggly lines in the DarwiinRemoteOSC
 *      window if you shake the WiiMote.
 * 
 * Instructions for Linux:
 *    - Install wiiosc (http://www.nescivi.nl/wiiosc/).
 *    - Launch wiiosc from the command line, with the following parameters:
 *         wiiosc 5600 57150 127.0.0.1 auto
 *      Pair your WiiMote with your computer (i.e., push 1 and 2 on the WiiMote at the same time).  After
 *      a few seconds, wiiosc should confirm the connection with the WiiMote.
 *      
 * Instructions for Windows:
 *    - Not sure; if you're lucky, you may be able to install wiiosc and use the Linux approach.
 * 
 * @author brinkman
 *
 */
public class WiiMoteOSC implements RawDevice, OSCListener {

	private static final String BATTERY = "/wii/batterylevel";
	private static final String ORIENTATION = "/wii/orientation";
	private static final String POINTER = "pointer";
	private static final String EVOLUTION = "evolution";

	private static final double THRESHOLD = 1e-3;

	private OSCServer osc;
	private Map<String, InputSlot> buttonSlots = new HashMap<String, InputSlot>();
	private InputSlot pointerSlot = null, evolutionSlot = null;
	private ToolEventQueue queue = null;

	private Matrix pointerMatrix = new Matrix();
	private Matrix evolutionMatrix = new Matrix();
	private DoubleArray pointerArray = new DoubleArray(pointerMatrix.getArray());
	private DoubleArray evolutionArray = new DoubleArray(evolutionMatrix.getArray());

	public void dispose() {
		osc.removeOSCListener(this);
		osc = null;
	}

	public String getName() {
		return "WiiMoteOSC";
	}

	public void initialize(Viewer viewer, Map<String, Object> config) {
		try {
			int port = 5600; // by default, DarwiinRemote sends events to port 5600
			if (config.containsKey("port"))  {
				Object co = config.get("port");
				if (co instanceof Integer) {
					port = (Integer) co;
				}
			}
			osc = OSCPool.getUDPServer(port);
			osc.addOSCListener(this);
		} catch (IOException e) {
			e.printStackTrace();
		}
	}

	public ToolEvent mapRawDevice(String rawDeviceName, InputSlot inputDevice) {
		if (EVOLUTION.equals(rawDeviceName)) {
			evolutionSlot = inputDevice;
			Matrix m0 = new Matrix();
			m0.setEntry(2, 3, -1);
			return new ToolEvent(this, System.currentTimeMillis(), inputDevice, new DoubleArray(m0.getArray()));
		} else if (POINTER.equals(rawDeviceName)) {
			pointerSlot = inputDevice;
			return new ToolEvent(this, System.currentTimeMillis(), inputDevice, new DoubleArray(new Matrix().getArray()));
		} else {
			buttonSlots.put(rawDeviceName, inputDevice);
			return new ToolEvent(this, System.currentTimeMillis(), inputDevice, AxisState.ORIGIN);
		}
	}

	public void setEventQueue(ToolEventQueue queue) {
		this.queue = queue;
	}

	public void messageReceived(OSCMessage msg, SocketAddress sender, long time) {
		if (queue==null) {
			return;
		}
		String name = msg.getName();
		if (!name.startsWith("/wii/")) {
			return;
		}
		if (ORIENTATION.equals(name)) {
			if (pointerSlot!=null || evolutionSlot!=null) {
				Object arg = msg.getArg(0);
				float roll = (arg instanceof Float) ? (Float) arg : (Integer) arg;
				arg = msg.getArg(1);
				float pitch = (arg instanceof Float) ? (Float) arg : (Integer) arg;
				orientationEvent(roll, pitch);
			}
		} else if (BATTERY.equals(name)) {
			showBattery((Float) msg.getArg(0));
		} else {
			InputSlot slot = buttonSlots.get(name);
			if (slot!=null) {
				buttonEvent(slot, (Integer) msg.getArg(0));
			} else if (!name.equals("/wii/acc")) {
				handleLinuxEvent(msg);
			}
		}
	}

	private float x, y, z;
	private static final float RAD2DEG = 180/(float) Math.PI;
	private Map<String, Integer> buttonStates = new HashMap<String, Integer>();

	private void handleLinuxEvent(OSCMessage msg) {
		try {
			int id = (Integer) msg.getArg(0);
			if (id!=0) return;
		} catch(Exception e) {
			return;
		}
		String name = msg.getName();
		if (name.startsWith("/wii/keys/")) {
			String key = name.substring(10);
			Integer state = (Integer) msg.getArg(1);
			if (!state.equals(buttonStates.get(key))) {
				buttonStates.put(key, state);
				InputSlot slot = buttonSlots.get("/wii/button/"+key);
				if (slot!=null) {
					buttonEvent(slot, state);
				}
			}
		} else if (name.equals("/wii/acc/x")) {
			x = 10*((Float) msg.getArg(1))-5;
		} else if (name.equals("/wii/acc/y")) {
			y = 10*((Float) msg.getArg(1))-5;
		} else if (name.equals("/wii/acc/z")) {
			z = 10*((Float) msg.getArg(1))-5;
			float pitch = (float) Math.atan2(y, z)*RAD2DEG;
			float roll = (float) Math.atan2(x, z)*RAD2DEG;
			orientationEvent(roll, pitch);
		} else if (name.equals("/wii/battery")) {
			showBattery((Float) msg.getArg(1));
		}
	}

	private void showBattery(float level) {
		System.err.println("WiiMote battery level: "+level);
	}

	private void buttonEvent(InputSlot slot, int status) {
		ToolEvent ev = new ToolEvent(this, System.currentTimeMillis(), slot, (status==0) ? AxisState.ORIGIN : AxisState.PRESSED);
		queue.addEvent(ev);
	}

	private double x0 = 0, y0 = 0;

	private void orientationEvent(float roll, float pitch) {
		x0 += (roll/75-x0)*0.08;
		y0 += (-pitch/75-y0)*0.08;

		x0 = (x0<-1) ? -1 : ((x0>1) ? 1 : x0);
		y0 = (y0<-1) ? -1 : ((y0>1) ? 1 : y0);

		if (evolutionSlot!=null) {
			evolutionEvent(x0, y0);
		}
		if (pointerSlot!=null) {
			matrixEvent(x0, y0);
		}
	}

	@SuppressWarnings("serial")
	private void matrixEvent(double x, double y) {
		pointerMatrix.setEntry(0, 3, x);
		pointerMatrix.setEntry(1, 3, y);
		pointerMatrix.setEntry(2, 3, -1);
		queue.addEvent(new ToolEvent(this, System.currentTimeMillis(), pointerSlot, pointerArray) {
			protected boolean compareTransformation(DoubleArray trafo1, DoubleArray trafo2) {
				return true;
			}
		});
	}

	@SuppressWarnings("serial")
	private void evolutionEvent(double x, double y) {
		double dx = Math.pow(x, 3)*.1;  // dynamic response: slow when close to neutral position
		double dy = Math.pow(y, 3)*.1;

		if (Math.abs(dx)<THRESHOLD && Math.abs(dy)<THRESHOLD) {
			return;
		}

		evolutionMatrix.setEntry(0, 3, dx);
		evolutionMatrix.setEntry(1, 3, dy);
		evolutionMatrix.setEntry(2, 3, -1);

		ToolEvent evolutionEvent = new ToolEvent(this, System.currentTimeMillis(), evolutionSlot, evolutionArray){
			protected boolean compareTransformation(DoubleArray trafo1, DoubleArray trafo2) {
				return true;
			}
			protected void replaceWith(ToolEvent replacement) {
				Matrix m = new Matrix(replacement.getTransformation());
				m.multiplyOnRight(getTransformation().toDoubleArray(null));
				trafo = new DoubleArray(m.getArray());
				time = replacement.getTimeStamp();
			}
		};
		queue.addEvent(evolutionEvent);
	}
}

/*
/wii/connected , i
/wii/mousemode , i
/wii/button/a , i
/wii/button/b , i
/wii/button/up , i
/wii/button/down , i
/wii/button/left , i
/wii/button/right , i
/wii/button/minus , i
/wii/button/plus , i
/wii/button/home , i
/wii/button/one , i
/wii/button/two , i
/wii/acc , fff
/wii/orientation , ff
/wii/irdata , ffffffffffff
/wii/batterylevel , f
/nunchuk/joystick , ff
/nunchuk/button/z , i
/nunchuk/button/c , i
/nunchuk/acc , fff
/nunchuk/orientation , ff
 */
