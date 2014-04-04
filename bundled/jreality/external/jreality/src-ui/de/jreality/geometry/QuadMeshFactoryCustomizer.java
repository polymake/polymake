package de.jreality.geometry;




/** A user interface for a {@link QuadMeshSetFactory}.
 * 
 * @author G. Paul Peters, 03.06.2010
 */
public class QuadMeshFactoryCustomizer extends AbstractGeometryFactoryCustomizer<QuadMeshFactory> {
	private static final long serialVersionUID = 1L;
	
	public QuadMeshFactoryCustomizer() {
		super();
	}
	
	public QuadMeshFactoryCustomizer(QuadMeshFactory factory) {
		super(factory);
	}
	
	Class<QuadMeshFactory> getAcceptableClass() {
		return QuadMeshFactory.class;
	}
	
	protected void initSliderProperties() {
		super.initSliderProperties();
		sliderProperties.add(new DoubleSliderProperty("uTextureScale", 0., 10.));
		sliderProperties.add(new DoubleSliderProperty("vTextureScale", 0., 10.));
		sliderProperties.add(new DoubleSliderProperty("uTextureShift", 0., 1.));
		sliderProperties.add(new DoubleSliderProperty("vTextureShift", 0., 1.));
	}
	
	protected void initToggleProperties() {
		super.initToggleProperties();
		toggleProperties.add(new ToggleProperty("generateVertexNormals", "vrtxNrmls"));
		toggleProperties.add(new ToggleProperty("generateFaceNormals", "fcNrmls"));
		toggleProperties.add(new ToggleProperty("generateTextureCoordinates", "txCrds"));
		toggleProperties.add(new ToggleProperty("generateEdgesFromFaces", "edges"));
		toggleProperties.add(new ToggleProperty("edgeFromQuadMesh", "cnctEdgs"));
		toggleProperties.add(new ToggleProperty("closedInUDirection", "uClosed"));
		toggleProperties.add(new ToggleProperty("closedInVDirection", "vClosed"));
		toggleProperties.add(new ToggleProperty("generateVertexLabels", "vrtxLbls"));
		toggleProperties.add(new ToggleProperty("generateEdgeLabels", "edgeLbls"));
		toggleProperties.add(new ToggleProperty("generateFaceLabels", "faceLbls"));
		toggleProperties.add(new ToggleProperty("generateAABBTree", "AABBTree"));	}
}
