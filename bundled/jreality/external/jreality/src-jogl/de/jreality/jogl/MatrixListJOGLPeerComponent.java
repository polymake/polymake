package de.jreality.jogl;

import javax.media.opengl.GL;
import javax.media.opengl.GL2;
import javax.media.opengl.GLException;

import de.jreality.math.Rn;
import de.jreality.scene.Geometry;
import de.jreality.scene.SceneGraphPath;
import de.jreality.scene.event.SceneGraphComponentEvent;
import de.jreality.util.CameraUtility;
import de.jreality.util.LoggingSystem;

/**
 * This is an alternative class for use in conjunction with special sorts of
 * scene graphs with many identical children, as one finds in discrete groups.
 * <p>
 * Rather than traversing the many identical children, the transformations of
 * all the children are provided in an array via a <i>backdoor</i> mechanism.
 * Furthermore, a separate boolean array of the same length is provided which
 * directs whether the corresponding matrix is visible.
 * <p>
 * A second main challenge is to generate display lists that include the whole
 * scene graph under the special scene graph component which is the "root" of
 * the discrete group.
 * <p>
 * If you are sensitive to highly optimized code, shield your eyes!
 * <p>
 * To activate use, run java with "-DdiscreteGroup.copycat=true" set
 * 
 * @author Charles Gunn
 * 
 */
public class MatrixListJOGLPeerComponent extends JOGLPeerComponent {

	boolean displayListDirty = true, isCopyCat = false, isTopCat = false,
			isCameraRepn = false, visibilityChanged = false, insideDL = false;
	double[][] matrices = null;
	boolean[] accepted;
	SceneGraphPath w2camrepn = null;
	double[] world2CameraRepn = new double[16],
			camera2CameraRepn = new double[16];
	int framecount = -1;
	protected boolean[] matrixIsReflection = null;
	GL oldGL;
	public static int instanceCount;
	MatrixListData theDropBox;

	public MatrixListJOGLPeerComponent() {
		super();
		instanceCount++;
	}

	public MatrixListJOGLPeerComponent(SceneGraphPath sgp, JOGLPeerComponent p,
			JOGLRenderer jr) {
		super(sgp, p, jr);
		instanceCount++;
	}

	@Override
	public void init(SceneGraphPath sgp, JOGLPeerComponent p, JOGLRenderer jr) {
		init(null, sgp, p, jr);
	}

	public void init(GoBetween gb, SceneGraphPath sgp, JOGLPeerComponent p,
			JOGLRenderer jr) {
		super.init(gb, sgp, p, jr);
		Geometry trojanHorse = goBetween.originalComponent.getGeometry();
		isCopyCat = (trojanHorse != null && trojanHorse instanceof MatrixListData);
		if (isCopyCat) {
			goBetween.peerGeometry = null; // this isn't really a geometry
			theDropBox = (MatrixListData) trojanHorse;
			readMatrices();
		}
	}

	private void readMatrices() {
		matrices = theDropBox.getMatrixList();
		matrixIsReflection = new boolean[matrices.length];
		accepted = new boolean[matrices.length];
		for (int i = 0; i < matrices.length; ++i) {
			matrixIsReflection[i] = Rn.determinant(matrices[i]) < 0.0;
		}

		// System.err.println("Updating matrices length "+matrices.length);
	}

	@Override
	public void render() throws GLException {
		if (!isVisible)
			return;
		if (childCount == 0 && goBetween.peerGeometry == null)
			return;
		insideDL = false;
		if (isCopyCat) {
			if (matrices.length != theDropBox.getMatrixList().length) {
				readMatrices();
				displayListDirty = true;
			}
			// if (displayListDirty) setDisplayListDirty();
			// figure out whether to use display lists
			if ((theDropBox.isNewVisibleList() || localVisibleList == null)) {
				localVisibleList = theDropBox.getVisibleList().clone();
				theDropBox.setNewVisibleList(false);
				displayListDirty = true;
				// System.err.println("Got visible list");
			} else if (jr.perfMeter.beginRenderTime
					- lastDisplayListCreationTime > theDropBox.getDelay()) {
				displayListDirty = true;
			}
			jr.renderingState.componentDisplayLists = theDropBox
					.isComponentDisplayLists();
			if (jr.renderingState.componentDisplayLists
					&& jr.renderingState.useDisplayLists && !jr.offscreenMode) {
				if (oldGL != jr.globalGL) {
					displayListDirty = true;
					oldGL = jr.globalGL;
				}
				// System.err.println("dirty subgraph = "+dirtySubGraph);
				if (!someSubNodeIsDirty()) { // we can't use the current display
												// list AND we can't generate a
												// new one
					if (!displayListDirty) {
						jr.globalGL.glCallList(displayList);
						// System.err.println("dgjoglpc: Calling dlist");
						return;
					}
					if (jr.renderingState.insideDisplayList) {
						// throw new
						// IllegalStateException("Already inside display list");
						printUpState();
					}
					if (displayList < 0)
						displayList = jr.globalGL.glGenLists(1);
					jr.globalGL.glNewList(displayList, GL2.GL_COMPILE);
					insideDL = jr.renderingState.insideDisplayList = true;
					// System.err.println("dlist elapsed time: "+(jr.perfMeter.beginRenderTime-lastDisplayListCreationTime));
					lastDisplayListCreationTime = jr.perfMeter.beginRenderTime;
					// theLog.fine("Turning on dlist for "+goBetween.getOriginalComponent().getName());
				}
			}
		}
		try {
			super.render();
		} catch (GLException e) {
			System.err.println("Got exception " + name);
			e.printStackTrace();
			if (theDropBox != null)
				setDisplayListDirty();
			else
				throw e;
		}
		if (insideDL) {
			jr.globalGL.glEndList();
			jr.globalGL.glCallList(displayList);
		}
		displayListDirty = false;
	}

	@Override
	protected boolean someSubNodeIsDirty() {
		if (!isVisible)
			return false;
		if (!isCopyCat) {
			if (displayListDirty) {
				// System.err.println(name+" is dldirty");
				return true;
			}
			if (goBetween.peerGeometry != null && geometryDirtyBits != 0) {
				return true;
			}
			for (JOGLPeerComponent child : children) {
				if (child.someSubNodeIsDirty())
					return true;
			}
			return false;
		}
		// the copy cat node only looks at his first child
		JOGLPeerComponent child = children.get(0);
		return (child.someSubNodeIsDirty());
	}

	private void printUpState() {
		if (debug)
			theLog.info("dld\tcld:\t" + displayListDirty + "\t" + "\t" + name);
		if (parent != null)
			((MatrixListJOGLPeerComponent) parent).printUpState();
	}

	int metric;
	double[] o2c, o2ndc;
	private boolean[] localVisibleList;

	@Override
	protected void renderChildren() {
		if (isCopyCat) {
			metric = jr.renderingState.currentMetric;
			if (debug)
				theLog.fine("In renderChildren()" + name);
			boolean isReflectionBefore = jr.renderingState.flipped; // cumulativeIsReflection;
			int nn = matrices.length;
			theDropBox.setRendering(true);
			boolean clipToCamera = theDropBox.isClipToCamera()
					&& !jr.offscreenMode;

			int count = 0;
			MatrixListJOGLPeerComponent child = (MatrixListJOGLPeerComponent) children
					.get(0);
			// an optimatization hack: do polygon shader here, set flag so
			// shading below this node is skipped
			if (!theDropBox.isShadeGeometry()) {
				geometryShader.polygonShader.render(jr.renderingState);
				jr.renderingState.shadeGeometry = false;
			}
			for (int j = 0; j < nn; ++j) {
				if (clipToCamera) {
					if (!localVisibleList[j])
						continue;
				}
				count++;
				cumulativeIsReflection = (isReflectionBefore ^ matrixIsReflection[j]);
				if (cumulativeIsReflection != jr.renderingState.flipped) {
					jr.globalGL.glFrontFace(cumulativeIsReflection ? GL.GL_CW
							: GL.GL_CCW);
					jr.renderingState.flipped = cumulativeIsReflection;
				}
				pushTransformation(matrices[j]);
				child.render();
				popTransformation();
			}
			if (!theDropBox.isShadeGeometry()) {
				geometryShader.polygonShader.postRender(jr.renderingState);
				jr.renderingState.shadeGeometry = true;
			}
			theDropBox.setRendering(false);
			jr.renderingState.flipped = isReflectionBefore;
			jr.globalGL.glFrontFace(jr.renderingState.flipped ? GL.GL_CW
					: GL.GL_CCW);
			jr.renderingState.componentDisplayLists = false;

		} else {
			super.renderChildren();
		}
	}

	@Override
	protected void pushTransformation(double[] m) {
		if (w2camrepn != null) {
			// recalculate the matrix if we haven't yet done it this frame
			if (framecount != jr.perfMeter.frameCount) {
				w2camrepn.getInverseMatrix(world2CameraRepn);
				framecount = jr.perfMeter.frameCount;
				Rn.times(camera2CameraRepn, world2CameraRepn,
						jr.renderingState.cameraToWorld);
				Rn.times(camera2CameraRepn, camera2CameraRepn,
						CameraUtility.inverseCameraOrientation.getArray());
				isReflection = Rn.determinant(camera2CameraRepn) < 0;
				isIdentity = Rn.isIdentityMatrix(camera2CameraRepn, 10E-8);

				// System.err.println("Setting up final thing");
			}
			super.pushTransformation(camera2CameraRepn);
		} else
			super.pushTransformation(m);
	}

	@Override
	protected void propagateAppearanceChanged() {
		displayListDirty = true;
		super.propagateAppearanceChanged();
	}

	@Override
	protected void updateShaders() {
		super.updateShaders();
		if (eAp == null)
			return;
		if (goBetween.originalComponent.getAppearance() != null) {
			Object foo = goBetween.originalComponent.getAppearance()
					.getAttribute("discreteGroup.cameraRep",
							SceneGraphPath.class);
			if (foo != null && foo instanceof SceneGraphPath) {
				w2camrepn = (SceneGraphPath) foo;
				LoggingSystem.getLogger(this).finer(
						"Found path in " + w2camrepn);
				useTformCaching = false;
			} else
				w2camrepn = null;
		}
		if (!isCopyCat)
			return;
		isTopCat = (isCopyCat && !existsHigherCat());
	}

	private boolean existsHigherCat() {
		if (parent != null)
			if (((MatrixListJOGLPeerComponent) parent).isCopyCat)
				return true;
			else
				return ((MatrixListJOGLPeerComponent) parent).existsHigherCat();
		else
			return false;
	}

	@Override
	protected void updateTransformationInfo() {
		// propagateSGCDisplayListDirtyUp();
		displayListDirty = true;
		super.updateTransformationInfo();
	}

	@Override
	void setDisplayListDirty() {
		displayListDirty = true;
		super.setDisplayListDirty();
	}

	@Override
	public void visibilityChanged(SceneGraphComponentEvent ev) {
		if (debug)
			theLog.fine("Visibility changed: " + name);
		// propagateSGCDisplayListDirtyUp();
		displayListDirty = true;
		super.visibilityChanged(ev);
	}

}
