/*
 * Created on Jan 14, 2007
 *
 */
package de.jreality.jogl;

import java.util.logging.Logger;

import de.jreality.util.LoggingSystem;

public class JOGLPeerNode {
	String name;
	JOGLRenderer jr;
	static Logger theLog = LoggingSystem.getLogger(JOGLPeerNode.class);

	public JOGLPeerNode() {
		super();
	}

	public JOGLPeerNode(JOGLRenderer jr) {
		super();
		this.jr = jr;
	}

	public String getName() {
		return name;
	}

	public void setName(String n) {
		name = n;
	}
}
