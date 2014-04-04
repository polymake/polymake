package de.jreality.toolsystem.raw;

import java.util.Map;

import de.jreality.math.Rn;
import de.jreality.openhaptics.OHRawDevice;
import de.jreality.openhaptics.OHViewer;
import de.jreality.scene.Viewer;
import de.jreality.scene.data.DoubleArray;
import de.jreality.scene.tool.AxisState;
import de.jreality.scene.tool.InputSlot;
import de.jreality.toolsystem.ToolEvent;
import de.jreality.toolsystem.ToolEventQueue;
import de.jreality.ui.viewerapp.ViewerSwitch;
import de.jtem.jopenhaptics.HL;
import de.jtem.jopenhaptics.HLeventProc;

public class PhantomDesktop implements RawDevice, OHRawDevice, HLeventProc {
	boolean deviceActive=true;
	
	InputSlot[] slots = new InputSlot[2];
	
	private ToolEventQueue queue;
	
	private OHViewer ohviewer;

	public String getName() {
		return "PhantomDesktop";
	}

	
	public void initialize(Viewer viewer, Map<String, Object> config) {
		if(viewer instanceof ViewerSwitch){
			for(Viewer v : ((ViewerSwitch) viewer).getViewers()){
				if(v instanceof OHViewer){
					viewer = v;
					break;
				}
			}
		}
		
		if(viewer instanceof OHViewer ){
			ohviewer =((OHViewer)viewer);
			ohviewer.getRawDevices().add(this);
			ohviewer.setBox((double[]) config.get("p0"),(double[]) config.get("p1"));
		} else throw new IllegalArgumentException("The phantomdesktop device requires the open haptics viewer (OHViewer).");
	}

	public ToolEvent mapRawDevice(String rawDeviceName, InputSlot inputDevice) {
		if (rawDeviceName.equals("trafo"))
			slots[0]=inputDevice;
		else if (rawDeviceName.equals("button_1"))
			slots[1]=inputDevice;
		else 
			throw new IllegalArgumentException("no such device: " + rawDeviceName);
		System.out.println("registered "+rawDeviceName+"->"+inputDevice);
		return new ToolEvent(PhantomDesktop.this, System.currentTimeMillis(), inputDevice, AxisState.ORIGIN);
	}

	public void setEventQueue(ToolEventQueue queue) {
		this.queue = queue;
	}
	
	public void initHaptic() {
		deviceActive = ohviewer != null && ohviewer.getRenderer().isDevicePresent();
		if(!deviceActive){
			return;
		}
		else{
			HL.hlAddEventCallback(HL.HL_EVENT_1BUTTONUP, HL.HL_OBJECT_ANY, HL.HL_COLLISION_THREAD, new HLeventProc() {
				public void eventProc(long event, int object, long thread, long cache) {
					queue.addEvent(new ToolEvent(PhantomDesktop.this, System.currentTimeMillis(), slots[1], AxisState.ORIGIN));
				}
			});
			HL.hlAddEventCallback(HL.HL_EVENT_1BUTTONDOWN, HL.HL_OBJECT_ANY, HL.HL_COLLISION_THREAD, new HLeventProc() {
				public void eventProc(long event, int object, long thread, long cache) {
					queue.addEvent(new ToolEvent(PhantomDesktop.this, System.currentTimeMillis(), slots[1], AxisState.PRESSED));
				}
			});
			HL.hlAddEventCallback(HL.HL_EVENT_MOTION, HL.HL_OBJECT_ANY, HL.HL_COLLISION_THREAD, this);
		}
	}

	public void dispose() {
	}

	public void eventProc(long event, int object, long thread, long cache) {
		double trafo[] = new double[16];
		HL.hlCacheGetDoublev(cache, HL.HL_PROXY_TRANSFORM, trafo, 0);
		
		queue.addEvent(new ToolEvent(PhantomDesktop.this, System.currentTimeMillis(), slots[0], new DoubleArray(Rn.transpose(null, trafo))) {
			private static final long serialVersionUID = 5542511510287252014L;
			@Override
			protected boolean compareTransformation(DoubleArray trafo1, DoubleArray trafo2) {
				return true;
			}
		});
	}
}
