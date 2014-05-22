package de.jreality.ui;

import java.awt.GridBagConstraints;
import java.awt.Insets;

public class LayoutFactory {

	public static GridBagConstraints createLeftConstraint() {
		GridBagConstraints c1 = new GridBagConstraints();
		c1.insets = new Insets(1,1,1,1);
		c1.fill = GridBagConstraints.BOTH;
		c1.anchor = GridBagConstraints.WEST;
		c1.weightx = 1.0;
		c1.gridwidth = 1;
		return c1;
	}
	
	public static GridBagConstraints createRightConstraint() {
		GridBagConstraints c2 = new GridBagConstraints();
		c2.insets = new Insets(1,1,1,1);
		c2.fill = GridBagConstraints.BOTH;
		c2.anchor = GridBagConstraints.WEST;
		c2.weightx = 1.0;
		c2.gridwidth = GridBagConstraints.REMAINDER;
		return c2;
	}
	
}
