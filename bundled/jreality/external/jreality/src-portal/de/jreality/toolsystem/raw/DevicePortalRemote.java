package de.jreality.toolsystem.raw;

import java.util.Map;

import de.jreality.math.Matrix;
import de.jreality.scene.Viewer;
import de.jreality.scene.data.DoubleArray;
import de.jreality.scene.tool.AxisState;
import de.jreality.scene.tool.InputSlot;
import de.jreality.toolsystem.ToolEvent;
import de.jreality.toolsystem.ToolEventQueue;

public class DevicePortalRemote implements RawDevice {

	public void dispose() {
	}

	public String getName() {
		return "Remote";
	}

	public void initialize(Viewer viewer, Map<String, Object> config) {
	}

	public ToolEvent mapRawDevice(String rawDeviceName, InputSlot inputDevice) {
		if (rawDeviceName.equals("tick")) { // System time
			return new ToolEvent(this, -1, inputDevice, AxisState.ORIGIN);
		}
		if (rawDeviceName.startsWith("button") || rawDeviceName.startsWith("valuator")) { // Wand button
			return new ToolEvent(this, -1, inputDevice, AxisState.ORIGIN);
		}
		if (rawDeviceName.startsWith("sensor")) {
			return new ToolEvent(this, -1, inputDevice, new DoubleArray(new Matrix().getArray()));
		}
		throw new IllegalArgumentException("unhandled remote device: "+rawDeviceName+" mapped to "+inputDevice);
	}

	ToolEventQueue queue;
	
	public void setEventQueue(ToolEventQueue queue) {
		this.queue=queue;
	}

}
