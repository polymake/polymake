package de.jreality.toolsystem.raw;

import java.util.Arrays;

import de.jreality.util.NativePathUtility;

import net.java.games.input.Controller;
import net.java.games.input.ControllerEnvironment;
import de.jreality.util.NativePathUtility;

public class DetectControllers {
	public static void main(String[] args) {
		NativePathUtility.set("jni");
		ControllerEnvironment env = ControllerEnvironment.getDefaultEnvironment();
		Controller[] controllers = env.getControllers();
		for (Controller ctrl : controllers) {
			System.out.println(ctrl+" :: "+Arrays.toString(ctrl.getComponents()));
		}
	}
}
