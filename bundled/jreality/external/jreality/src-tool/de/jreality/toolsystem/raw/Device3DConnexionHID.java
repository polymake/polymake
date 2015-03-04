package de.jreality.toolsystem.raw;

import java.io.IOException;
import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;
import java.util.logging.Level;
import java.util.logging.Logger;

import com.codeminders.hidapi.ClassPathLibraryLoader;
import com.codeminders.hidapi.HIDDevice;
import com.codeminders.hidapi.HIDDeviceInfo;
import com.codeminders.hidapi.HIDManager;

import de.jreality.scene.Viewer;
import de.jreality.scene.tool.AxisState;
import de.jreality.scene.tool.InputSlot;
import de.jreality.toolsystem.ToolEvent;
import de.jreality.toolsystem.ToolEventQueue;

public class Device3DConnexionHID implements RawDevice, PollingDevice {

	private static Logger	
		log = Logger.getLogger(Device3DConnexionHID.class.getName());
	private ToolEventQueue
		toolEventQueue = null;
	private HIDDevice
		device = null;
	private byte[]
		pollBuffer = new byte[50];
	private Map<String, InputSlot>
		slotMapping = new HashMap<String, InputSlot>();
	private Set<InputSlot> 
		pressedButtons = new HashSet<InputSlot>();
	
	private static final int
		BUTTON_RELEASE = 0,
		BUTTON_FIT = 1 << 10;
	
	static {
		ClassPathLibraryLoader.loadNativeHIDLibrary();
	}
	
	public Device3DConnexionHID() {
		try {
			device = createDevice();
			if (device != null) {
				device.disableBlocking();
			}
		} catch (Exception e) {
			log.log(Level.WARNING, "could not obtain 3DConnexion device");
		}
	}
	
	protected void process(byte[] data, long when) {
		switch (data[0]) {
		case 1: // translation
			InputSlot isX = slotMapping.get("x");
			if (isX != null) {
				double val = toNormalizedCoordinate(data[1], data[2]);
				AxisState newState = new AnalogAxisState(val);
				ToolEvent e = new ToolEvent(this, when, isX, newState);
				toolEventQueue.addEvent(e);
			}
			InputSlot isY = slotMapping.get("y");
			if (isX != null) {
				double val = toNormalizedCoordinate(data[3], data[4]);
				AxisState newState = new AnalogAxisState(val);
				ToolEvent e = new ToolEvent(this, when, isY, newState);
				toolEventQueue.addEvent(e);
			}
			InputSlot isZ = slotMapping.get("z");
			if (isX != null) {
				double val = toNormalizedCoordinate(data[5], data[6]);
				AxisState newState = new AnalogAxisState(val);
				ToolEvent e = new ToolEvent(this, when, isZ, newState);
				toolEventQueue.addEvent(e);
			}			
			break;
		case 2: // rotation
			InputSlot isRX = slotMapping.get("rx");
			if (isRX != null) {
				double val = toNormalizedCoordinate(data[1], data[2]);
				AxisState newState = new AnalogAxisState(val);
				ToolEvent e = new ToolEvent(this, when, isRX, newState);
				toolEventQueue.addEvent(e);
			}
			InputSlot isRY = slotMapping.get("ry");
			if (isRX != null) {
				double val = toNormalizedCoordinate(data[3], data[4]);
				AxisState newState = new AnalogAxisState(val);
				ToolEvent e = new ToolEvent(this, when, isRY, newState);
				toolEventQueue.addEvent(e);
			}
			InputSlot isRZ = slotMapping.get("rz");
			if (isRX != null) {
				double val = toNormalizedCoordinate(data[5], data[6]);
				AxisState newState = new AnalogAxisState(val);
				ToolEvent e = new ToolEvent(this, when, isRZ, newState);
				toolEventQueue.addEvent(e);
			}
			break;
		case 3: // button
			int id = toDeviceID(data[1], data[2]);
			switch (id) {
			case BUTTON_FIT:
				InputSlot buttonFitInput = slotMapping.get("button_fit");
				if (buttonFitInput != null) {
					pressedButtons.add(buttonFitInput);
					AxisState pressedState = new AxisState(1.0);
					ToolEvent e = new ToolEvent(this, when, buttonFitInput, pressedState);
					toolEventQueue.addEvent(e);
				}
				break;
			case BUTTON_RELEASE:
				for (InputSlot buttonIn : pressedButtons) {
					AxisState pressedState = new AxisState(0.0);
					ToolEvent e = new ToolEvent(this, when, buttonIn, pressedState);
					toolEventQueue.addEvent(e);
				}
				pressedButtons.clear();
				break;
			}
		default:
			break;
		}
	}
	
	protected double toNormalizedCoordinate(byte lsb, byte msb) {
		int high = msb << 8;
		int low =  (int)lsb & 0xFF;
		int ival = high | low;
		return ival / 1043.0;
	}
	
	protected int toDeviceID(byte lsb, byte msb) {
		return msb << 8 + lsb;
	}
	
	
    private class AnalogAxisState extends AxisState {

		private static final long serialVersionUID = 1L;

		public AnalogAxisState(double value) {
			super(value);
		}

		@Override
		public boolean isPressed() {
			return intValue() != 0;
		}
		
    }
	
	@Override
	public void poll(long when) {
		if (device == null) return;
		try {
			int read = device.read(pollBuffer);
			if (read != 0) {
				process(Arrays.copyOf(pollBuffer, read), when);
			}
		} catch (IOException e) {
			log.log(Level.WARNING, "error while polling device", e);
		}
	}

	@Override
	public ToolEvent mapRawDevice(String rawDeviceName, InputSlot inputDevice) {
		slotMapping.put(rawDeviceName, inputDevice);
		long when = System.currentTimeMillis();
		return new ToolEvent(this, when, inputDevice, AxisState.ORIGIN);
	}
	
	
	protected HIDDevice createDevice() throws Exception {
		HIDManager m = HIDManager.getInstance();
		HIDDeviceInfo ad = null;
		for (HIDDeviceInfo d : m.listDevices()) {
			if (d.getManufacturer_string().contains("3Dconnexion")) {
				ad = d;
			}
		}
		return m.openByPath(ad.getPath());
	}
	
	@Override
	public void setEventQueue(ToolEventQueue queue) {
		this.toolEventQueue = queue;
	}

	@Override
	public void initialize(Viewer viewer, Map<String, Object> config) {
		
	}

	@Override
	public void dispose() {
		
	}

	@Override
	public String getName() {
		return getClass().getSimpleName();
	}
	
}
