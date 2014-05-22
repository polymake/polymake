package de.jreality.macosx.sms;


public class SMSLib {
	
	//	- Didn't match model name
	public static final int FAIL_MODEL			= (-7);
	//	- Failure getting dictionary matching desired services
	public static final int FAIL_DICTIONARY		= (-6);
	//	- Failure getting list of services
	public static final int FAIL_LIST_SERVICES	= (-5);
	//	- Failure if list of services is empty. The process generally fails
	//		here if run on a machine without a Sudden Motion Sensor.
	public static final int FAIL_NO_SERVICES	= (-4);
	//	- Failure if error opening device.
	public static final int FAIL_OPENING		= (-3);
	//	- Failure if opened, but didn't get a connection
	public static final int FAIL_CONNECTION		= (-2);
	//	- Failure if couldn't access connction using given function and size. This
	//		is where the process would probably fail with a change in Apple's API.
	//		Driver problems often also cause failures here.
	public static final int FAIL_ACCESS			= (-1);
	//	- Success!
	public static final int SUCCESS				= (0);
	
	
	static {
		// ensure native JNI library is loaded
		System.loadLibrary("SMSLib");
		Runtime.getRuntime().addShutdownHook(new Thread() {
			public void run() {
				shutdown();
			}
		});
	}
		
	public static native float[] getValues();
	
	private static native int init();
	
	private static native void shutdown();
	
	public static void initSMS() {
		int ret = init();
		checkResult(ret);
	}
	
	private static void checkResult(int res) {
		switch (res) {
			case SUCCESS:
				return;
			case FAIL_ACCESS:
				throw new RuntimeException("FAIL_ACCESS");
			case FAIL_CONNECTION:
				throw new RuntimeException("FAIL_CONNECTION");
			case FAIL_OPENING:
				throw new RuntimeException("FAIL_OPENING");
			case FAIL_NO_SERVICES:
				throw new RuntimeException("FAIL_NO_SERVICES");
			case FAIL_LIST_SERVICES:
				throw new RuntimeException("FAIL_LIST_SERVICES");
			case FAIL_DICTIONARY:
				throw new RuntimeException("FAIL_DICTIONARY");
			case FAIL_MODEL:
				throw new RuntimeException("FAIL_MODEL");
			default:
				throw new RuntimeException("Unmatched return value: "+res);
		}
	}
	
}
