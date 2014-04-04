package de.jreality.swing;

import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.ComponentAdapter;
import java.awt.event.ComponentEvent;

import javax.swing.JFrame;

import de.jreality.geometry.IndexedFaceSetFactory;
import de.jreality.geometry.IndexedLineSetFactory;
import de.jreality.math.Matrix;
import de.jreality.math.MatrixBuilder;
import de.jreality.scene.Appearance;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.tool.InputSlot;
import de.jreality.scene.tool.ToolContext;
import de.jreality.shader.CommonAttributes;
import de.jreality.tools.ActionTool;

public class ScenePanel {
	private boolean inScene = true;
	private JFrame externalFrame = new JFrame();
	SceneGraphComponent rootNode = new SceneGraphComponent(); // translated in front of the avatar
	SceneGraphComponent rackNode = new SceneGraphComponent();
	SceneGraphComponent panelNode = new SceneGraphComponent();

	Appearance app = new Appearance();

	JFakeFrame frame = new JFakeFrame();

	IndexedFaceSetFactory panel = new IndexedFaceSetFactory();
	IndexedLineSetFactory rack = new IndexedLineSetFactory();

	double[][] panelVerts = { { -.5, 0, 0 }, { -.5, 1, 0 }, { .5, 1, 0 }, { .5, 0, 0 } };
	double[][] tcs = { { 1, 1 }, { 1, 0 }, { 0, 0 }, { 0, 1 } };
	int[][] rackEdgeIndices = { { 4, 0 }, { 0, 1 }, { 1, 5 }, { 5, 4 }, { 0, 3 }, { 1, 2 }, { 4, 7 }, { 5, 6 } };

	double aboveGround = 1.8;
	double belowGround = -.2;
	double panelWidth = 1;
	double angle = .8 * Math.PI / 2;
	double zOffset = -2.5;

	ActionTool myActionTool=new ActionTool("PanelActivation");

	public ScenePanel() {
		try {
			externalFrame.setAlwaysOnTop(true);
		} catch (SecurityException se) {
			// webstart
		}
		rootNode.setName("panel");
		panel.setVertexCount(4);
		panel.setFaceCount(1);
		panel.setVertexCoordinates(panelVerts);
		panel.setVertexTextureCoordinates(tcs);
		panel.setFaceIndices(new int[][] { { 0, 1, 2, 3 } });
		panel.setGenerateVertexNormals(true);
		panel.update();
		panelNode.setGeometry(panel.getIndexedFaceSet());

		Appearance panelApp = frame.getAppearance();
		panelNode.setAppearance(frame.getAppearance());
		panelNode.addTool(frame.getTool());

		panelApp.setAttribute("lineShader.tubeDraw", false);
		panelApp.setAttribute(CommonAttributes.LIGHTING_ENABLED, false);
		panelApp.setAttribute(CommonAttributes.VERTEX_DRAW, false);

		frame.addComponentListener(new ComponentAdapter() {
			public void componentResized(ComponentEvent e) {
				update();
			}

			public void componentShown(ComponentEvent e) {
				update();
				rootNode.setVisible(true);
			}

			public void componentHidden(ComponentEvent e) {
				rootNode.setVisible(false);
			}
		});

		myActionTool.addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent e) {
				toggle((ToolContext) e.getSource());
			}
		});

		rack.setVertexCount(8);
		rack.setEdgeCount(8);
		rack.setEdgeIndices(rackEdgeIndices);
		rackNode.setGeometry(rack.getIndexedLineSet());

		app.setAttribute("showLines", true);
		app.setAttribute("lineShader.tubeRadius", 0.008);
		app.setAttribute(CommonAttributes.POINT_SHADER+"."+CommonAttributes.AMBIENT_COEFFICIENT,.1);
		app.setAttribute(CommonAttributes.LINE_SHADER+"."+CommonAttributes.AMBIENT_COEFFICIENT,.1);
		app.setAttribute("lineShader.diffuseColor", java.awt.Color.gray);
		app.setAttribute("pointShader.pointRadius", 0.016);
		app.setAttribute("pointShader.diffuseColor", java.awt.Color.gray);
		app.setAttribute("showPoints", true);

		rackNode.setAppearance(app);

		rootNode.addChild(rackNode);
		rackNode.addChild(panelNode);

		MatrixBuilder.euclidean().rotateY(Math.PI).assignTo(rackNode);

		rootNode.setVisible(false);
	}

	public void setPanelWidth(double panelWidth) {
		this.panelWidth=panelWidth;
		update();
	}

	void update() {
		int width=frame.getWidth(); int height=frame.getHeight();
		double panelHeight = panelWidth * height / width;
		rack.setVertexCoordinates(rackVerts(panelHeight));
		rack.update();
		MatrixBuilder.euclidean().translate(0,
				aboveGround - Math.sin(angle) * panelHeight, 0).rotateX(
						Math.PI / 2 - angle).scale(panelWidth, panelHeight, 1).assignTo(
								panelNode);
	}

	boolean showFeet=true;

	double[][] rackVerts(double panelHeight) {
		double cos = Math.cos(angle);
		double sin = Math.sin(angle);
		double[][] verts = new double[8][3];

		// upper right back
		verts[0][0] = panelWidth/2;
		verts[0][1] = aboveGround;
		verts[0][2] = cos * panelHeight;

		// upper right front
		verts[1][0] = panelWidth/2;
		verts[1][1] = aboveGround - sin * panelHeight;
		verts[1][2] = 0;

		// lower right front
		verts[2][0] = panelWidth/2;
		verts[2][1] = showFeet ? belowGround : aboveGround - sin * panelHeight;;
		verts[2][2] = 0;

		// lower right back
		verts[3][0] = panelWidth/2;
		verts[3][1] = showFeet ? belowGround : aboveGround;
		verts[3][2] = cos * panelHeight;

		// upper left back
		verts[4][0] = -panelWidth/2;
		verts[4][1] = aboveGround;
		verts[4][2] = cos * panelHeight;

		// upper left front
		verts[5][0] = -panelWidth/2;
		verts[5][1] = aboveGround - sin * panelHeight;
		verts[5][2] = 0;

		// lower left front
		verts[6][0] = -panelWidth/2;
		verts[6][1] = showFeet ? belowGround : aboveGround - sin * panelHeight;
		verts[6][2] = 0;

		// lower left back
		verts[7][0] = -panelWidth/2;
		verts[7][1] = showFeet ? belowGround : aboveGround;
		verts[7][2] = cos * panelHeight;

		return verts;
	}

	public JFrame getFrame() {
		return inScene ? frame : externalFrame;
	}
	
	public JFrame getExternalFrame() {
		return externalFrame;
	}

	public SceneGraphComponent getComponent() {
		return rootNode;
	}

	public void show(ToolContext tc) {
		if (inScene) {
			Matrix avatar = new Matrix(tc.getTransformationMatrix(InputSlot.getDevice("AvatarTransformation")));
			show(tc.getViewer().getSceneRoot(), avatar);
		} else {
			externalFrame.setVisible(true);
		}
	}

	public void show(SceneGraphComponent component, Matrix avatar) {
		if (inScene) {
			setPosition(avatar);
			if (!component.isDirectAncestor(getComponent())) {
				component.addChild(getComponent());
			}
			frame.setVisible(true);
		} else {
			externalFrame.setVisible(true);
		}
	}

	public void setPosition(Matrix avatar) {
		MatrixBuilder.euclidean(avatar).translate(0, 0, getZOffset()).assignTo(rootNode);
	}

	public void hide(SceneGraphComponent cmp) {
		if (inScene) {
			if (cmp.isDirectAncestor(getComponent())) {
				cmp.removeChild(getComponent());
			}
			frame.setVisible(false);
		} else {
			externalFrame.setVisible(false);
		}
	}
	
	public void hide(ToolContext tc) {
		hide(tc.getViewer().getSceneRoot());
	}

	public void toggle(ToolContext tc) {
		toggle(tc.getViewer().getSceneRoot(), new Matrix(tc.getAvatarPath().getMatrix(null)));
	}
	
	public void toggle(SceneGraphComponent component, Matrix avatar) {
		if (inScene) {
			if (frame.isVisible()) hide(component);
			else show(component, avatar);
		} else {
			externalFrame.setVisible(!externalFrame.isVisible());
		}
	}  

	public ActionTool getPanelTool() {
		return myActionTool;
	}

	public double getAboveGround() {
		return aboveGround;
	}

	public void setAboveGround(double aboveGround) {
		this.aboveGround = aboveGround;
		update();
	}

	public double getAngle() {
		return angle;
	}

	public void setAngle(double angle) {
		this.angle = angle;
		update();
	}

	public double getBelowGround() {
		return belowGround;
	}

	public void setBelowGround(double belowGround) {
		this.belowGround = belowGround;
		update();
	}

	public double getZOffset() {
		return zOffset;
	}

	/**
	 * Note: this affects the next show call but does not change the
	 * current position of the panel.
	 * 
	 * @param offset the new zOffset
	 */
	public void setZOffset(double offset) {
		zOffset = offset;
	}

	public double getPanelWidth() {
		return panelWidth;
	}

	public boolean isShowFeet() {
		return showFeet;
	}

	public void setShowFeet(boolean showFeet) {
		this.showFeet = showFeet;
	}

	public void adjustHeight(double delta) {
		MatrixBuilder.euclidean(rootNode).translate(0, delta, 0).assignTo(rootNode);
	}

	public boolean isInScene() {
		return inScene;
	}

	public void setInScene(boolean b, SceneGraphComponent cmp, Matrix m) {
		if (inScene == b) return;
		
		if (b) {
			inScene = b;
			boolean visible = externalFrame.isVisible();
			externalFrame.setVisible(false);
			frame.setContentPane(externalFrame.getContentPane());
			externalFrame.remove(externalFrame.getContentPane());
			frame.pack();
			if (visible) show(cmp,m);
		} else {
			boolean visible = frame.isVisible();
			hide(cmp);
			inScene = b;
			externalFrame.setContentPane(frame.getContentPane());
			frame.remove(frame.getContentPane());
			externalFrame.pack();
			if (visible) {
				externalFrame.setVisible(true);
			}
		}
		
	}
}
