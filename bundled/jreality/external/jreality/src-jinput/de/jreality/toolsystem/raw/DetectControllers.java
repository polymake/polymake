package de.jreality.toolsystem.raw;

import java.util.Arrays;

import net.java.games.input.Controller;
import net.java.games.input.ControllerEnvironment;

public class DetectControllers {
	public static void main(String[] args) {
		ControllerEnvironment env = ControllerEnvironment.getDefaultEnvironment();
		Controller[] controllers = env.getControllers();
		for (Controller ctrl : controllers) {
			System.out.println(ctrl+" :: "+Arrays.toString(ctrl.getComponents()));
		}
	}
}
