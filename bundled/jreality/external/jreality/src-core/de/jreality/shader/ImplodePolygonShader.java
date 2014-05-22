/*
 * Created on Sep 5, 2008
 *
 */
package de.jreality.shader;

public interface ImplodePolygonShader  extends DefaultPolygonShader {

	  Object CREATE_DEFAULT=new Object();

	public static final Double IMPLODE_FACTOR_DEFAULT = -0.6;
	public Double getImplodeFactor();
	public void setImplodeFactor(Double d);

}
