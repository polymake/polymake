package de.jreality.scene.tool;

import java.util.Arrays;
import java.util.Collections;

/**
 * This class is a workaround for problems with constructors having arguments
 * in the beanshell interpreter. It should become obsolete with improved versions
 * of beanshell.
 * 
 * @author Ulrich
 *
 */
public class BeanShellTool extends AbstractTool {
	public BeanShellTool() {
		super((InputSlot) null);
	}
	
	public void setActivationSlots(InputSlot[] slots) {
		if (slots == null || slots.length == 0 || slots[0] == null)
			activationSlots=Collections.emptyList();
		else
			activationSlots=Arrays.asList(slots);
	}
}
