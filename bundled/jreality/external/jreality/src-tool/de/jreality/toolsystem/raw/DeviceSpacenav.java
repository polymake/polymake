package de.jreality.toolsystem.raw;

import java.io.IOException;
import java.util.Map;

import net.sf.spacenav.SpaceNav;
import net.sf.spacenav.SpaceNavButtonEvent;
import net.sf.spacenav.SpaceNavEvent;
import net.sf.spacenav.SpaceNavMotionEvent;
import de.jreality.scene.Viewer;
import de.jreality.scene.tool.AxisState;
import de.jreality.scene.tool.InputSlot;
import de.jreality.toolsystem.ToolEvent;
import de.jreality.toolsystem.ToolEventQueue;

public class DeviceSpacenav implements RawDevice, Runnable {
	
	boolean running=true;

	private SpaceNav s;

	double x, y, z, rx, ry, rz;
	
	// 6 axes, 2 buttons x, y, z, rx, ry, rz, bLeft, bRight
	InputSlot[] slots = new InputSlot[8];
	
	private ToolEventQueue queue;

	private double sensitivity;

	private volatile long timestamp = System.currentTimeMillis();
	
	synchronized boolean isRunning() {
		return running;
	}
	
	synchronized void stop() {
		running = false;
	}
	
	public void dispose() {
		stop();
	}

	public String getName() {
		return "Spacenav";
	}

	public void initialize(Viewer viewer, Map<String, Object> config) {
		s=new SpaceNav();
		sensitivity = 1.0;
		if (config.containsKey("sensitivity")) {
			sensitivity = (Double) config.get("sensitivity");
		}
		// this is not this.sensitivity!!
	    s.setSensitivity(1.0);
	    
	    new Thread(this).start();
	    
	    if (config.containsKey("screensaver-wakeup-cmd")) {
			final String SCREENSAVER_WAKEUP_CMD = (String) config.get("screensaver-wakeup-cmd");
		    Thread t = new Thread(
		    	new Runnable() {
					public void run() {
						while (isRunning()) {
							synchronized (DeviceSpacenav.this) {
								try {
									DeviceSpacenav.this.wait(1000);
								} catch (InterruptedException e1) {
								}
							}
							long ct = System.currentTimeMillis();
							int diff = (int) (ct - timestamp);
							try {
								if (diff < 2500) Runtime.getRuntime().exec(SCREENSAVER_WAKEUP_CMD);
							} catch (IOException e) {
								System.out.println("Could not trigger screensaver - giving up.");
								return;
							}
						}
					}
		    	});
		    t.setPriority(Thread.MIN_PRIORITY);
		    t.start();
	    }
	}

	public ToolEvent mapRawDevice(String rawDeviceName, InputSlot inputDevice) {
		if (rawDeviceName.equals("x"))
			slots[0]=inputDevice;
		else if (rawDeviceName.equals("y"))
			slots[1]=inputDevice;
		else if (rawDeviceName.equals("z"))
			slots[2]=inputDevice;
		else if (rawDeviceName.equals("rx"))
			slots[3]=inputDevice;
		else if (rawDeviceName.equals("ry"))
			slots[4]=inputDevice;
		else if (rawDeviceName.equals("rz"))
			slots[5]=inputDevice;
		else if (rawDeviceName.equals("buttonLeft"))
			slots[6]=inputDevice;
		else if (rawDeviceName.equals("buttonRight"))
			slots[7]=inputDevice;
		else 
			throw new IllegalArgumentException("no such device: "+rawDeviceName);
		System.out.println("registered "+rawDeviceName+"->"+inputDevice);
		return new ToolEvent(this, System.currentTimeMillis(), inputDevice, AxisState.ORIGIN);
	}

	public void setEventQueue(ToolEventQueue queue) {
		this.queue = queue;
	}

	public void button(SpaceNavButtonEvent e) {
		InputSlot slot = slots[6+e.getButton()];
		if (slot != null) queue.addEvent(new ToolEvent(this, System.currentTimeMillis(), slot, e.isPressed() ? AxisState.PRESSED : AxisState.ORIGIN));
	}

	public void motion(SpaceNavMotionEvent e) {
//		System.out.println("DeviceSpacenav.motion()");
//		
//		System.out.printf("Motion event: tx:%4d ty:%4d tz:%4d rx:%4d ry:%4d rz:%4d\n",
//			    e.getX(), e.getY(), e.getZ(),
//			    e.getRX(), e.getRY(), e.getRZ()
//			);
//		
		double cx, cy, cz, crx, cry, crz;
		cx = e.getX();
		if (cx!=x) {
			x=cx;
			queue.addEvent(new ToolEvent(this, timestamp, slots[0], new AxisState(sensitivity*x)));
		}
		cy = e.getY();
		if (cy!=y){
			y=cy;
			queue.addEvent(new ToolEvent(this, timestamp, slots[1], new AxisState(sensitivity*y)));

		}
		cz = e.getZ();
		if (cz!=z){
			z=cz;
			queue.addEvent(new ToolEvent(this, timestamp, slots[2], new AxisState(sensitivity*z)));

		}
		crx = e.getRX();
		if (crx!=rx){
			rx=crx;
			queue.addEvent(new ToolEvent(this, timestamp, slots[3], new AxisState(sensitivity*rx)));

		}
		cry = e.getRY();
		if (cry!=ry){
			ry=cry;
			queue.addEvent(new ToolEvent(this, timestamp, slots[4], new AxisState(sensitivity*ry)));

		}
		crz = e.getRZ();
		if (crz!=rz){
			rz=crz;
			queue.addEvent(new ToolEvent(this, timestamp, slots[5], new AxisState(sensitivity*rz)));

		}
	}

	public void run() {
		SpaceNavEvent e;
		while((e=s.waitForEvent())!=null && isRunning()) {
			long ct = System.currentTimeMillis();
			long dt = ct - timestamp;
			timestamp = ct;
			if (dt > 1000) {
				synchronized(this) {
					// quit screensaver
					this.notifyAll();
				}
			}
			if(e instanceof SpaceNavMotionEvent) {
				SpaceNavMotionEvent m=(SpaceNavMotionEvent) e;
				motion(m);
			}
			else if(e instanceof SpaceNavButtonEvent) {
				SpaceNavButtonEvent b=(SpaceNavButtonEvent) e;
				button(b);
			}
			else {
				System.out.println("Unknown event!");
			}
		}
		s.closeDevice();
	}
	
	
}
