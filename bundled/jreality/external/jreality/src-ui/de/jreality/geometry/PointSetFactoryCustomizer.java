package de.jreality.geometry;



/** A user interface for a {@link PointSetFactory}.
 * 
 * @author G. Paul Peters, 03.06.2010
 */
public class PointSetFactoryCustomizer extends AbstractGeometryFactoryCustomizer<PointSetFactory> {
	private static final long serialVersionUID = 1L;
	
	public PointSetFactoryCustomizer() {
		super();
	}
	
	public PointSetFactoryCustomizer(PointSetFactory factory) {
		super(factory);
	}
	
	Class<PointSetFactory> getAcceptableClass() {
		return PointSetFactory.class;
	}
	
	
	protected void initToggleProperties() {
		super.initToggleProperties();
		toggleProperties.add(new ToggleProperty("generateVertexLabels", "vrtxLbls"));
	}
}
