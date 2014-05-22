package de.jreality.geometry;

/** A user interface for a {@link IndexedLineSetFactory}.
 * 
 * @author G. Paul Peters, 03.06.2010
 */
public class IndexedLineSetFactoryCustomizer extends AbstractGeometryFactoryCustomizer<IndexedLineSetFactory> {
	private static final long serialVersionUID = 1L;
	
	public IndexedLineSetFactoryCustomizer() {
		super();
	}
	
	public IndexedLineSetFactoryCustomizer(IndexedLineSetFactory factory) {
		super(factory);
	}
	
	Class<IndexedLineSetFactory> getAcceptableClass() {
		return IndexedLineSetFactory.class;
	}
	
	protected void initSliderProperties() {
		super.initSliderProperties();
	}
	
	protected void initToggleProperties() {
		super.initToggleProperties();
		toggleProperties.add(new ToggleProperty("generateVertexLabels", "vrtxLbls"));
		toggleProperties.add(new ToggleProperty("generateEdgeLabels", "edgeLbls"));
	}
}
