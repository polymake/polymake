package de.jreality.geometry;

import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.Map;

import de.jreality.scene.data.Attribute;
import de.jreality.scene.data.DataList;
import de.jreality.scene.data.DataListSet;
import de.jreality.scene.data.WritableDataList;

public class GeometryAttributeListSet {

	final AbstractGeometryFactory factory;

	final DataListSet DLS = new DataListSet(0);

	final HashMap  attributeNode = new HashMap();
	
	private final HashSet<Attribute> blockAttribute = new HashSet<Attribute>();
	
	final String category;

	boolean blockAllAttributes = false;
	
	
	boolean blockAttributeCount = false;
	
	GeometryAttributeListSet( AbstractGeometryFactory factory, String cathegory) {
		this.factory = factory;
		this.category = cathegory;
	}

	int noa() {
		return DLS.getListLength();
	}

	int getCount() {
		return noa();
	}

	void setCount(int count) {
		if (count == noa())
			return;

		if( blockAttributeCount )
			throw new UnsupportedOperationException
			( "The current state does not allow to change the count of the attribute category " + category  );
		
		DLS.reset(count);
	}

	private OoNode geometryAttributeNode(Map attributeNode, Attribute attr) {
		if (attributeNode.containsKey(attr))
			return (OoNode) attributeNode.get(attr);

		OoNode node = factory.node(category + "." + attr);

		attributeNode.put(attr, node);

		return node;
	}

	OoNode attributeNode(Attribute attr) {
		return this.geometryAttributeNode(attributeNode, attr);
	}

	void updateAttributes() {
		for (Iterator iter = DLS.storedAttributes().iterator(); iter.hasNext();) {
			Attribute attr = (Attribute) iter.next();
			attributeNode(attr).update();
		}
	}

	void setAttribute(Attribute attr, DataList data) {
		if( isBlockedAttribute(attr))
			throw new UnsupportedOperationException( "cannot set attribute " + attr );
		setAttrImpl(DLS, attr, data);
		attributeNode(attr).setObject(data);
	}

	boolean isBlockedAttribute(Attribute attr) {
		//System.out.println( "Category: " + category +  "  Attribute: " + attr +  "  " +blockAttribute.contains(attr) );
		return blockAllAttributes || blockAttribute.contains(attr);
	}

	void setAttributes(DataListSet dls) {
		for( Iterator iter = dls.storedAttributes().iterator(); iter.hasNext(); ) {
			Attribute attr = (Attribute)iter.next();
			if( isBlockedAttribute(attr))
				throw new UnsupportedOperationException( "cannot set attribute " + attr );
		}
		setAttrImpl(DLS, dls);
		for (Iterator iter = dls.storedAttributes().iterator(); iter.hasNext();) {
			Attribute attr = (Attribute) iter.next();
			attributeNode(attr).setObject(DLS.getList(attr));
		}
	}

	final void setAttrImpl(DataListSet target, DataListSet data) {
		if(target.getListLength() != data.getListLength() ) {  //TODO: what is the intention of this method?
			target.reset( data.getListLength() );
		}
		
		for (Iterator i = data.storedAttributes().iterator(); i.hasNext();) {
			Attribute a = (Attribute) i.next();
			setAttrImpl(target, a, data.getList(a));
		}
	}

	static final void setAttrImpl(DataListSet target, Attribute a, DataList d) {

		if (d == null) {
			target.remove(a);
		} else {
			WritableDataList w;
			w = target.getWritableList(a);
			// problem with indices; easiest way around is to remove old indices before setting new
			// we need a way to check if the complete 2D array of the new data 
			// is the same dimension as the old; if not, we get exception when a copy
			// lacking that, we assume they're different. note that the same fix is needed
			// in de.jreality.scene.Geometry.setAttrImpl(DataListSet target, Attribute a, DataList d, boolean replace)
			if (w == null || a == Attribute.INDICES) {
				w = target.addWritable(a, d.getStorageModel());
			} 	
			d.copyTo(w);
			
		}
	}

	public void blockAttribute( Attribute attr ) {
		blockAttribute.add(attr);
		//System.out.println( "  block Category: " + category +  "  Attribute: " + attr +  "  " +blockAttribute.contains(attr) );
	}
	
	public void unblockAttribute( Attribute attr ) {
		blockAttribute.remove(attr);
		//System.out.println( "unblock Category: " + category +  "  Attribute: " + attr +  "  " +blockAttribute.contains(attr) );
	}

	public boolean isBlockAllAttributes() {
		return blockAllAttributes;
	}

	public void setBlockAllAttributes(boolean blockAllAttributes) {
		this.blockAllAttributes = blockAllAttributes;
	}

	public boolean isBlockAttributeCount() {
		return blockAttributeCount;
	}

	public void setBlockAttributeCount(boolean blockAttributeCount) {
		this.blockAttributeCount = blockAttributeCount;
	}

	public boolean hasEntries() {
		return DLS.getNumAttributes() != 0 ;
	}
}

