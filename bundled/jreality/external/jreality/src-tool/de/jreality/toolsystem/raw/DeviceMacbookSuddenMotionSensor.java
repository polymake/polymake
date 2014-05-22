package de.jreality.toolsystem.raw;

import java.util.Map;

import de.jreality.macosx.sms.SMSLib;
import de.jreality.math.Matrix;
import de.jreality.scene.Viewer;
import de.jreality.scene.data.DoubleArray;
import de.jreality.scene.tool.InputSlot;
import de.jreality.toolsystem.ToolEvent;
import de.jreality.toolsystem.ToolEventQueue;

public class DeviceMacbookSuddenMotionSensor implements RawDevice, PollingDevice {
	
	InputSlot slot;
		
	Matrix mat = new Matrix();

	private ToolEventQueue queue;
	
	public void dispose() {
	}

	public String getName() {
		return "Mac SuddenMotionSensor";
	}

	public void initialize(Viewer viewer, Map<String, Object> config) {
		SMSLib.initSMS();
	}
	
	public ToolEvent mapRawDevice(String rawDeviceName, InputSlot inputDevice) {
		slot = inputDevice;
		return new ToolEvent(this, System.currentTimeMillis(), slot, new DoubleArray(mat.getArray()));
	}

	public void setEventQueue(ToolEventQueue queue) {
		this.queue = queue;
	}

	public void poll(long when) {
		float[] v = SMSLib.getValues();
		mat.setEntry(0, 3, v[0]);
		mat.setEntry(1, 3, v[1]);
		mat.setEntry(2, 3, v[2]);
		ToolEvent te = new ToolEvent(this, when, slot, new DoubleArray(mat.getArray()));
		queue.addEvent(te);
	}

}
