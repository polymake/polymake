package de.jreality.writer.u3d;

import de.jreality.scene.data.Attribute;
import de.jreality.scene.data.DataList;
import de.jreality.scene.data.DoubleArray;

public abstract class U3DAttribute {

	public static final DataList
		U3D_FLAG = new DoubleArray(new double[]{});
	
	public static final Attribute 
		U3D_NONORMALS = Attribute.attributeForName("U3D_ForceNormals");
	
}
