//
//  FooJNI.java
//  FooJNI
//
//  Created by Steffen Weissmann on 03.09.09.
//  Copyright (c) 2009 TU Berlin. All rights reserved.
//
//	For information on setting Java configuration information, including 
//	setting Java properties, refer to the documentation at
//		http://developer.apple.com/techpubs/java/java.html
//

package com.example;

import java.util.Arrays;


/**
 * Starting point for the application. General initialization should be done inside
 * the ApplicationController's init() method. If certain kinds of non-Swing initialization
 * takes too long, it should happen in a new Thread and off the Swing event dispatch thread (EDT).
 * 
 * @author weissman
 */
public class FooJNI {
	public static void main(final String[] args) {
				
		de.jreality.macosx.sms.SMSLib.initSMS();
		
		while(true) {
			float[] vals = de.jreality.macosx.sms.SMSLib.getValues();
			System.out.println("vals="+Arrays.toString(vals));
			try {
				Thread.sleep(200);
			}
			catch (Exception ex) {
				ex.printStackTrace();
			}
		}
		
	}
}
