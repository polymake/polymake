package de.jreality.geometry;

import java.util.Iterator;
import java.util.Map;

import de.jreality.geometry.OoNode.IsUpdateCounter;
import de.jreality.scene.Geometry;
import de.jreality.scene.Scene;
import de.jreality.scene.data.Attribute;
import de.jreality.scene.data.DataList;

/**
 * The grandfather of all the jReality geometry factories.  
 * <p>
 * Allows user to set the ambient metric via {@link #setMetric(int)}.
 * <p>
 * And provides most general get method with {@link #getGeometry()}.
 * <p>
 * This class writes itself into the associated {@link Geometry} using the call:
 * <code><pre>
 * geometry.setGeometryAttributes(GeometryUtility.FACTORY, this);
 * </code></pre>
 * so that later users of <i>geometry</i> can access the factory and use it for editing, etc.
 * <p>
 * For an introduction to how the factories work, see {@link PointSetFactory}.
 * 
 * @author gunn
 *
 */
public class AbstractGeometryFactory implements GeometryFactory {

	final OoNode metric;
	
	UpdateCounter update = new UpdateCounter();
	
	static class UpdateCounter implements IsUpdateCounter {
		long counter = 0;
		public long getUpdateCount() {
			return counter;
		}
	};
	
	
	final Geometry geometry;
	
	AbstractGeometryFactory( Geometry geometry, int metric ) {
		
		this.metric = node( new Integer( metric ), "metric" );
		this.geometry = geometry;
		GeometryUtility.setMetric( geometry,metric);
	}
	
	public Geometry getGeometry() {
		return geometry;
	}
	
	OoNode node(String name) {
		return new OoNode( name, update );
	}

	OoNode node(Object object, String name) {
		return new OoNode( object, name, update );
	}

	OoNode geometryAttributeNode(Map attributeNode, String name, Attribute attr) {
		if( attributeNode.containsKey(attr))
			return (OoNode)attributeNode.get( attr );
		
		OoNode node = node( name+"."+attr );
		
		attributeNode.put( attr, node );
		
		return node;
	}

	protected boolean nodeWasUpdated(OoNode node) {
		return node.getCounterOfLastUpdate() == update.getUpdateCount();
	}

	public int getMetric() {
		return ((Integer)metric.getObject()).intValue();
	}

	public void setMetric(int metric) {
		this.metric.setObject( new Integer( metric) );
	}

	/**
	 * Subclasses should add here all computations necessary for update.  
	 * This method is called by {@link #update()} before {@link #updateImpl()}.
	 * The actual update of the scene graph should happen in {@link #updateImpl()}.
	 */
	void recompute() {
	}

	/**
	 * This method is the API method for updating the geometry factory. 
	 */
	public void update() {
		update.counter++;
		recompute();
		
		Scene.executeWriter( geometry, new Runnable() {
			
			public void run() {
				
				updateImpl();
			}
		}
		);
		
	}

	/**
	 * Subclasses should add here all updates that affect the scene graph.
	 * This method is called
	 * by {@link #update()} after {@link #recompute()}.  
	 * It is called through {@link Scene#executeWriter()}
	 * so that the scene graph of the geometry is already locked.    
	 */
	void updateImpl() {
		GeometryUtility.setMetric( geometry, getMetric());	
	}

	void updateGeometryAttributeCathegory( GeometryAttributeListSet gals ) {
		if( !gals.hasEntries())
			return;
		
		Geometry geometry = gals.factory.geometry;
		String category = gals.category;
		
		if( geometry.getNumEntries( category ) == gals.noa() ) {
			
			for( Iterator iter = gals.DLS.storedAttributes().iterator(); iter.hasNext(); ) {
				Attribute attr = (Attribute)iter.next();
				gals.attributeNode( attr ).update();
				if(  nodeWasUpdated(gals.attributeNode( attr ))  ) {
					log( "set", attr, category);
					geometry.setAttributes( category, attr, gals.DLS.getWritableList(attr));
				}
			}
		} else {
			gals.updateAttributes();
			geometry.setCountAndAttributes( category, gals.DLS);		
		}
	}
	
	void updateNode( GeometryAttributeListSet gals, Attribute attr, boolean generate, OoNode node ) {
		Geometry geometry = gals.factory.geometry;
		String category = gals.category;
			
		if (generate) {
			if (nodeWasUpdated(node)) {
				log("set", attr, category);
				DataList dl = node.createDataList();
				geometry.setAttributes(category, attr, dl );
			}
		} else if (!gals.DLS.containsAttribute(attr)
				&& geometry.getAttributes(gals.category, attr) != null) {
			log("cancel", attr, category);
			geometry.setAttributes(category, attr, null);
		}	
	}
	
	String logMessage(String action, String attr, String cathegory) {
		return action + " " + cathegory + " " + attr;
	}

	void log(String action, Attribute attr, String cathegory) {
		log(action, attr.getName(), cathegory);
	}

	boolean debug = false;

	void log(String action, String attr, String cathegory) {
	//		if( actionLogger != null ) {
	//			actionLogger.log( Level.INFO, logMessage(action, attr, cathegory),
	//					new Object[] {action, attr, cathegory } );
	//		}
			if( debug )
				System.out.println( logMessage( action, attr, cathegory ) );
		}

}
