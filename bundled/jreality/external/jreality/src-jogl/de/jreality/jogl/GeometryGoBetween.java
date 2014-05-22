package de.jreality.jogl;

import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.util.HashMap;
import java.util.WeakHashMap;
import java.util.logging.Level;

import javax.swing.Timer;

import de.jreality.scene.Geometry;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SceneGraphVisitor;

public class GeometryGoBetween {

	JOGLRenderer jr;
	Timer followTimer;
	boolean geometryRemoved = false;
	boolean checkMemoryLeak = false;

	protected GeometryGoBetween(JOGLRenderer jr) {
		this.jr = jr;
		followTimer = new Timer(1000, new ActionListener() {
			public void actionPerformed(ActionEvent e) {
				updateGeometryHashtable();
			}
		});
		if (checkMemoryLeak)
			followTimer.start();
	}

	public void dispose() {
		followTimer.setRepeats(false);
		followTimer.stop();
		followTimer = null;
	}

	int geomDiff = 0;
	protected HashMap<Geometry, JOGLPeerGeometry> geometries = new HashMap<Geometry, JOGLPeerGeometry>();

	protected void updateGeometryHashtable() {
		if (!geometryRemoved)
			return;
		final WeakHashMap<Geometry, JOGLPeerGeometry> newG = new WeakHashMap<Geometry, JOGLPeerGeometry>();
		SceneGraphVisitor cleanup = new SceneGraphVisitor() {
			public void visit(SceneGraphComponent c) {
				if (c.getGeometry() != null) {
					Geometry wawa = c.getGeometry();
					JOGLPeerGeometry peer = geometries.get(wawa);
					newG.put(wawa, peer);
				}
				c.childrenAccept(this);
			}
		};
		cleanup.visit(jr.theRoot);
		geometryRemoved = false;
		// TODO dispose of the peer geomtry nodes which are no longer in the
		// graph
		if (geometries.size() - newG.size() != geomDiff) {
			JOGLConfiguration.theLog.log(Level.WARNING, "Old, new hash size: "
					+ geometries.size() + " " + newG.size());
			geomDiff = geometries.size() - newG.size();
		}
		return;
	}

	public JOGLPeerGeometry getJOGLPeerGeometryFor(Geometry g) {
		JOGLPeerGeometry pg;
		synchronized (geometries) {
			pg = (JOGLPeerGeometry) geometries.get(g);
			if (pg != null)
				return pg;
			pg = new JOGLPeerGeometry(g, jr);
			geometries.put(g, pg);
		}
		return pg;
	}

}
