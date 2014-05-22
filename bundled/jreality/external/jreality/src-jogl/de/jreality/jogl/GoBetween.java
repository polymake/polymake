/*
 * Created on Jan 14, 2007
 *
 */
package de.jreality.jogl;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.WeakHashMap;
import java.util.logging.Level;

import de.jreality.scene.Appearance;
import de.jreality.scene.Geometry;
import de.jreality.scene.Lock;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.Transformation;
import de.jreality.scene.event.AppearanceEvent;
import de.jreality.scene.event.AppearanceListener;
import de.jreality.scene.event.GeometryEvent;
import de.jreality.scene.event.GeometryListener;
import de.jreality.scene.event.LightEvent;
import de.jreality.scene.event.LightListener;
import de.jreality.scene.event.SceneGraphComponentEvent;
import de.jreality.scene.event.SceneGraphComponentListener;
import de.jreality.scene.event.TransformationEvent;
import de.jreality.scene.event.TransformationListener;
import de.jreality.shader.CommonAttributes;
import de.jreality.util.LoggingSystem;

// register for geometry change events
//static Hashtable goBetweenTable = new Hashtable();
public class GoBetween extends JOGLPeerNode implements GeometryListener,
		TransformationListener, AppearanceListener,
		SceneGraphComponentListener, LightListener {
	SceneGraphComponent originalComponent;
	ArrayList<JOGLPeerComponent> peers = new ArrayList<JOGLPeerComponent>();
	JOGLPeerGeometry peerGeometry;
	Lock peersLock = new Lock();
	boolean singlePeer = false;

	protected static WeakHashMap<JOGLRenderer, HashMap> tableForJR = new WeakHashMap<JOGLRenderer, HashMap>();

	static Class<? extends GoBetween> gbClass = GoBetween.class;

	public static void setGoBetweenClass(Class<? extends GoBetween> c) {
		gbClass = c;
	}

	// we have to keep track of all the peers associated to each renderer
	// separately
	protected static WeakHashMap<JOGLRenderer, HashMap<SceneGraphComponent, GoBetween>> rendererTable = new WeakHashMap<JOGLRenderer, HashMap<SceneGraphComponent, GoBetween>>();

	public static GoBetween goBetweenFor(JOGLRenderer jr,
			SceneGraphComponent sgc, boolean singlePeer) {
		if (sgc == null)
			return null;
		HashMap<SceneGraphComponent, GoBetween> gbt = rendererTable.get(jr);
		if (gbt == null) {
			gbt = new HashMap<SceneGraphComponent, GoBetween>();
			rendererTable.put(jr, gbt);
		}
		GoBetween gb = gbt.get(sgc);
		if (gb != null)
			return gb;
		try {
			gb = gbClass.newInstance();
		} catch (InstantiationException e) {
			e.printStackTrace();
		} catch (IllegalAccessException e) {
			e.printStackTrace();
		}
		gb.init(sgc, jr, singlePeer);
		gbt.put(sgc, gb);
		return gb;
		// System.err.println("Already have gb for "+sgc.getName());
	}

	// need this standard constructor for the above method
	protected GoBetween() {
	}

	protected GoBetween(SceneGraphComponent sgc, JOGLRenderer jr,
			boolean inheritSinglePeer) {
		super();
		init(sgc, jr, inheritSinglePeer);
	}

	public void init(SceneGraphComponent sgc, JOGLRenderer jr,
			boolean inheritSinglePeer) {
		this.jr = jr;
		originalComponent = sgc;
		// System.err.println("Initializing peer for "+originalComponent.getName());
		if (originalComponent.getGeometry() != null) {
			peerGeometry = jr.geometryGB
					.getJOGLPeerGeometryFor(originalComponent.getGeometry());
			peerGeometry.refCount++;
			originalComponent.getGeometry().addGeometryListener(this);
			// System.err.println("Adding geom listener for "+originalComponent.getGeometry().getName());
		} else
			peerGeometry = null;
		originalComponent.addSceneGraphComponentListener(this);
		// System.err.println("adding listener for "+originalComponent.getName());
		singlePeer = inheritSinglePeer;
		if (originalComponent.getAppearance() != null) {
			originalComponent.getAppearance().addAppearanceListener(this);
		}
		if (originalComponent.getTransformation() != null)
			originalComponent.getTransformation().addTransformationListener(
					this);
		if (originalComponent.getLight() != null)
			originalComponent.getLight().addLightListener(this);
	}

	public void dispose() {
		// System.err.println("disposing "+originalComponent.getName());
		// if (singlePeer) return; // need to do reference counting!
		originalComponent.removeSceneGraphComponentListener(this);
		if (originalComponent.getAppearance() != null)
			originalComponent.getAppearance().removeAppearanceListener(this);
		if (peerGeometry != null) {
			originalComponent.getGeometry().removeGeometryListener(this);
			// System.err.println("Removing geom listener for "+originalComponent.getGeometry().getName());
			peerGeometry.dispose();
		}
		if (originalComponent.getTransformation() != null)
			originalComponent.getTransformation().removeTransformationListener(
					this);
		if (originalComponent.getLight() != null)
			originalComponent.getLight().removeLightListener(this);
		if (rendererTable.get(jr).containsKey(originalComponent)) {
			rendererTable.get(jr).remove(originalComponent);
			// System.err.println("removing go between for "+originalComponent.getName());
		}
	}

	public boolean isSinglePeer() {
		return singlePeer;
	}

	public JOGLPeerComponent getSinglePeer() {
		return (peers.size() == 0 ? null : peers.get(0));
	}

	public void addJOGLPeer(JOGLPeerComponent jpc) {
		if (peers.contains(jpc))
			return;
		peersLock.writeLock();
		peers.add(jpc);
		peersLock.writeUnlock();
	}

	public void removeJOGLPeer(JOGLPeerComponent jpc) {
		if (!peers.contains(jpc))
			return;
		peersLock.writeLock();
		peers.remove(jpc);
		peersLock.writeUnlock();

		if (peers.size() == 0) {
			theLog.log(Level.FINE,
					"GoBetween for " + originalComponent.getName()
							+ " has no peers left");
			HashMap<SceneGraphComponent, GoBetween> gbt = rendererTable.get(jr);

			gbt.remove(originalComponent);
			dispose();
		}
	}

	public JOGLPeerGeometry getPeerGeometry() {
		return peerGeometry;
	}

	public void geometryChanged(GeometryEvent ev) {
		// peersLock.readLock();
		LoggingSystem.getLogger(this).fine(
				"sgc " + originalComponent.getName() + " Geometry changed");
		for (JOGLPeerComponent peer : peers) {
			peer.setDisplayListDirty();
		}
		// peersLock.readUnlock();
	}

	public void transformationMatrixChanged(TransformationEvent ev) {
		// System.err.println("TForm event for "+originalComponent.getName());
		// peersLock.readLock();
		for (JOGLPeerComponent peer : peers) {
			peer.transformationMatrixChanged(ev);
		}
		// peersLock.readUnlock();
	}

	public void appearanceChanged(AppearanceEvent ev) {
		String key = ev.getKey();
		LoggingSystem.getLogger(this).fine(
				"sgc " + originalComponent.getName() + " Appearance changed "
						+ key);
		int changed = 0;
		boolean propagates = true;
		// TODO shaders should register keywords somehow and which geometries
		// might be changed
		if (key.indexOf(CommonAttributes.IMPLODE_FACTOR) != -1)
			changed |= (JOGLPeerComponent.FACES_CHANGED);
		else if (key.indexOf(CommonAttributes.TRANSPARENCY) != -1)
			changed |= (JOGLPeerComponent.ALL_GEOMETRY_CHANGED);
		else if (key.indexOf(CommonAttributes.SMOOTH_SHADING) != -1)
			changed |= (JOGLPeerComponent.ALL_GEOMETRY_CHANGED);
		else if (key.indexOf(CommonAttributes.TUBE_RADIUS) != -1)
			changed |= (JOGLPeerComponent.LINES_CHANGED);
		else if (key.indexOf(CommonAttributes.POINT_RADIUS) != -1)
			changed |= (JOGLPeerComponent.POINTS_CHANGED);
		else if (key.indexOf(CommonAttributes.LEVEL_OF_DETAIL) != -1)
			changed |= (JOGLPeerComponent.POINTS_CHANGED);
		else if (key.indexOf(CommonAttributes.ANY_DISPLAY_LISTS) != -1)
			changed |= (JOGLPeerComponent.ALL_GEOMETRY_CHANGED);
		else if (key.endsWith("Shader"))
			changed |= JOGLPeerComponent.ALL_SHADERS_CHANGED;
		else if (key.endsWith("Shadername"))
			changed |= JOGLPeerComponent.ALL_SHADERS_CHANGED;
		// there are some appearances which we know aren't inherited, so don't
		// propagate change event.
		else if (key.indexOf(CommonAttributes.TEXTURE_2D) != -1)
			changed |= (JOGLPeerComponent.FACES_CHANGED);
		if (key.indexOf(CommonAttributes.PICKABLE) != -1
				|| key.indexOf(CommonAttributes.BACKGROUND_COLOR) != -1
				|| key.indexOf("fog") != -1)
			propagates = false;

		// peersLock.readLock();
		for (JOGLPeerComponent peer : peers) {
			if (propagates)
				peer.appearanceChanged(ev);
			if (changed != 0)
				peer.propagateGeometryChanged(changed);
		}
		// peersLock.readUnlock();
		// theLog.log(Level.FINER,"setting display list dirty flag: "+changed);
	}

	public void childAdded(SceneGraphComponentEvent ev) {
		theLog.log(Level.FINE, "GoBetween: Container Child added to: "
				+ originalComponent.getName());
		if (ev.getChildType() == SceneGraphComponentEvent.CHILD_TYPE_GEOMETRY) {
			if (peerGeometry != null) {
				((Geometry) ev.getOldChildElement())
						.removeGeometryListener(this);
				peerGeometry.dispose();
				jr.geometryGB.geometryRemoved = true;
				theLog.log(Level.WARNING,
						"Adding geometry while old one still valid");
				peerGeometry = null;
			}
			if (originalComponent.getGeometry() != null) {
				peerGeometry = jr.geometryGB
						.getJOGLPeerGeometryFor(originalComponent.getGeometry());
				originalComponent.getGeometry().addGeometryListener(this);
				peerGeometry.refCount++;
			}
		} else if (ev.getChildType() == SceneGraphComponentEvent.CHILD_TYPE_TRANSFORMATION) {
			if (originalComponent.getTransformation() != null)
				originalComponent.getTransformation()
						.addTransformationListener(this);
		} else if (ev.getChildType() == SceneGraphComponentEvent.CHILD_TYPE_APPEARANCE) {
			if (originalComponent.getAppearance() != null)
				originalComponent.getAppearance().addAppearanceListener(this);
		}
		peersLock.readLock();
		for (JOGLPeerComponent peer : peers) {
			// peer.addSceneGraphComponentEvent(ev);
			peer.childAdded(ev);
		}
		peersLock.readUnlock();
	}

	public void childRemoved(SceneGraphComponentEvent ev) {
		theLog.log(Level.FINE, "GoBetween: Container Child removed: "
				+ originalComponent.getName());
		if (ev.getChildType() == SceneGraphComponentEvent.CHILD_TYPE_GEOMETRY) {
			if (peerGeometry != null) {
				((Geometry) ev.getOldChildElement())
						.removeGeometryListener(this);
				peerGeometry.dispose(); // really decreases reference count
				peerGeometry = null;
				jr.geometryGB.geometryRemoved = true;
			}
			// return;
		} else if (ev.getChildType() == SceneGraphComponentEvent.CHILD_TYPE_TRANSFORMATION) {
			if (originalComponent.getTransformation() != null)
				((Transformation) ev.getOldChildElement())
						.removeTransformationListener(this);
		} else if (ev.getChildType() == SceneGraphComponentEvent.CHILD_TYPE_APPEARANCE) {
			if (originalComponent.getAppearance() != null)
				((Appearance) ev.getOldChildElement())
						.removeAppearanceListener(this);
		}
		peersLock.readLock();
		for (JOGLPeerComponent peer : peers) {
			peer.childRemoved(ev);
		}
		peersLock.readUnlock();
	}

	public void childReplaced(SceneGraphComponentEvent ev) {
		theLog.log(Level.FINE, "GoBetween: Container Child replaced: "
				+ originalComponent.getName());
		if (ev.getChildType() == SceneGraphComponentEvent.CHILD_TYPE_GEOMETRY) {
			if (peerGeometry != null
					&& peerGeometry.originalGeometry == originalComponent
							.getGeometry())
				return; // no change, really
			if (peerGeometry != null) {
				((Geometry) ev.getOldChildElement())
						.removeGeometryListener(this);
				peerGeometry.dispose();
				jr.geometryGB.geometryRemoved = true;
				peerGeometry = null;
			}
			if (originalComponent.getGeometry() != null) {
				originalComponent.getGeometry().addGeometryListener(this);
				peerGeometry = jr.geometryGB
						.getJOGLPeerGeometryFor(originalComponent.getGeometry());
				peerGeometry.refCount++;
			}
		} else if (ev.getChildType() == SceneGraphComponentEvent.CHILD_TYPE_TRANSFORMATION) {
			if (originalComponent.getTransformation() != null)
				originalComponent.getTransformation()
						.addTransformationListener(this);
		} else if (ev.getChildType() == SceneGraphComponentEvent.CHILD_TYPE_APPEARANCE) {
			if (originalComponent.getAppearance() != null)
				originalComponent.getAppearance().addAppearanceListener(this);
		} else if (ev.getChildType() == SceneGraphComponentEvent.CHILD_TYPE_LIGHT) {
			if (originalComponent.getLight() != null)
				originalComponent.getLight().addLightListener(this);
			jr.lightListDirty = true;
		}
		peersLock.readLock();
		for (JOGLPeerComponent peer : peers) {
			// peer.addSceneGraphComponentEvent(ev);
			peer.childReplaced(ev);
		}
		peersLock.readUnlock();
	}

	public void visibilityChanged(SceneGraphComponentEvent ev) {
		// peersLock.readLock();
		for (JOGLPeerComponent peer : peers) {
			peer.visibilityChanged(ev);
		}
		// peersLock.readUnlock();
	}

	public void lightChanged(LightEvent ev) {
		jr.lightListDirty = true;
	}

}
