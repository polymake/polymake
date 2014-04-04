package de.jreality.geometry;


/** A user interface for a {@link IndexedFaceSetFactory}.
 * 
 * @author G. Paul Peters, 03.06.2010
 */
public class IndexedFaceSetFactoryCustomizer extends AbstractGeometryFactoryCustomizer<IndexedFaceSetFactory> {
	private static final long serialVersionUID = 1L;
	
	public IndexedFaceSetFactoryCustomizer() {
		super();
	}
	
	public IndexedFaceSetFactoryCustomizer(IndexedFaceSetFactory factory) {
		super(factory);
	}
	
	Class<IndexedFaceSetFactory> getAcceptableClass() {
		return IndexedFaceSetFactory.class;
	}
	
	protected void initSliderProperties() {
		super.initSliderProperties();
	}
	
	protected void initToggleProperties() {
		super.initToggleProperties();
		toggleProperties.add(new ToggleProperty("generateVertexNormals", "vrtxNrmls"));
		toggleProperties.add(new ToggleProperty("generateFaceNormals", "fcNrmls"));
		toggleProperties.add(new ToggleProperty("generateEdgesFromFaces", "edges"));
		toggleProperties.add(new ToggleProperty("generateVertexLabels", "vrtxLbls"));
		toggleProperties.add(new ToggleProperty("generateEdgeLabels", "edgeLbls"));
		toggleProperties.add(new ToggleProperty("generateFaceLabels", "faceLbls"));
		toggleProperties.add(new ToggleProperty("generateAABBTree", "AABBTree"));
	}
}
