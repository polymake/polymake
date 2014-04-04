package de.jreality.swing;

import java.awt.Color;
import java.awt.GraphicsConfiguration;
import java.awt.HeadlessException;
import java.awt.Rectangle;

import javax.swing.JRootPane;
import javax.swing.plaf.RootPaneUI;
import javax.swing.plaf.metal.MetalRootPaneUI;

import de.jreality.geometry.IndexedFaceSetFactory;
import de.jreality.math.MatrixBuilder;
import de.jreality.scene.Appearance;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.tool.InputSlot;
import de.jreality.scene.tool.Tool;
import de.jreality.shader.CommonAttributes;

public class JFakeFrameWithGeometry extends JFakeFrame {
	 
	private static final long serialVersionUID = 1L;
	SceneGraphComponent windowComponent;
	IndexedFaceSetFactory quadFactory;
	
	Rectangle normalBounds;

	public JFakeFrameWithGeometry() {
		super();
	}

	public JFakeFrameWithGeometry(GraphicsConfiguration gc) {
		super(gc);
	}
	
	public JFakeFrameWithGeometry(String title, GraphicsConfiguration gc) {
		super(title, gc);
	}
	
	public JFakeFrameWithGeometry(String title) throws HeadlessException {
		super(title);
	}
	
	 private static InputSlot drag0 = InputSlot.getDevice("PanelAction");
	  private static InputSlot drag2 = InputSlot.getDevice("PanelSelection");
	  private static InputSlot drag1 = InputSlot.getDevice("PanelMenu");
	private int desktopWidth=800;
	private int desktopHeight=600;
	private SceneGraphComponent windowRoot;
	private int layer;

	
	private class MyRootPane extends JRootPane {
		
		private static final long 
			serialVersionUID = 1L;
		private RootPaneUI
			rootPaneUI = new MetalRootPaneUI();
		
		public MyRootPane() {
			setUI(rootPaneUI);
		}
		
		@Override
		public RootPaneUI getUI() {
			return rootPaneUI;
		}
		
	}
	
	
	@Override
	protected void init() {
		setRootPane(new MyRootPane());
		getRootPane().setWindowDecorationStyle(JRootPane.FRAME);
    	setUndecorated(true);
    	
        normalBounds=new Rectangle();

        Tool leftMouseButtonTool = new PlanarMouseEventTool(drag0, 0, this);
        Tool centerMouseButtonTool = new PlanarMouseEventTool(drag1, 1, this);
        Tool rightMouseButtonTool = new PlanarMouseEventTool(drag2, 2, this);
		
		  appearance = new Appearance();
		  appearance.setAttribute(CommonAttributes.DIFFUSE_COLOR,Color.WHITE);
		  appearance.setAttribute(CommonAttributes.VERTEX_DRAW, false);
		  appearance.setAttribute(CommonAttributes.EDGE_DRAW, false);
		  appearance.setAttribute(CommonAttributes.TUBES_DRAW, false);
		  appearance.setAttribute(CommonAttributes.LIGHTING_ENABLED, false);
		  appearance.setAttribute("polygonShader.reflectionMap:blendColor", new Color(255,255,255,65));
		  
        quadFactory = new IndexedFaceSetFactory();
		quadFactory.setVertexCount(4);
		quadFactory.setFaceCount(1);
		quadFactory.setGenerateFaceNormals(true);
		quadFactory.setFaceIndices(new int[][]{{0,1,2,3}});
		quadFactory.setVertexTextureCoordinates(new double[][]{{0,0},{1,0},{1,1},{0,1}});
		windowComponent = new SceneGraphComponent();
		windowComponent.setVisible(false);
		windowComponent.addTool(leftMouseButtonTool);
		windowComponent.addTool(centerMouseButtonTool);
		windowComponent.addTool(rightMouseButtonTool);
		//windowComponent.addTool(tool);
		windowComponent.setAppearance(getAppearance());
		windowComponent.setGeometry(quadFactory.getGeometry());
		
		setBounds(getBounds());
		
	}

	@Override
	public void setBounds(int x, int y, int w, int h) {
		super.setBounds(x, y, w, h);
		if (windowComponent != null) {
			if (getExtendedState() == NORMAL) normalBounds.setBounds(x,y,w,h);
			MatrixBuilder.euclidean().translate(x,y,-layer).assignTo(windowComponent);
			double[][] loc = new double[][]{{0,0,0},{w,0,0},{w,h,0},{0,h,0}};
			quadFactory.setVertexCoordinates(loc);
			quadFactory.update();
		}
	}
	
	@Override
	public void setVisible(boolean b) {
		super.setVisible(b);
		if (windowComponent != null) windowComponent.setVisible(b);
	}

	public SceneGraphComponent getSceneGraphComponent() {
		return windowComponent;
	}
	
	@Override
	public synchronized void setExtendedState(int state) {
		if ((state & ICONIFIED) != 0) {
			System.out.println("iconified not (yet) supported...");
			return;
		}
		super.setExtendedState(state);
		if ((state & MAXIMIZED_BOTH) != 0) {
			setBounds(0, 0, desktopWidth, desktopHeight);
		} else {
			if ((state & MAXIMIZED_HORIZ) != 0) {
				Rectangle obds = getBounds();
				setBounds(0, obds.y, 800, obds.width);
			}
			if ((state & MAXIMIZED_VERT) != 0) {
				Rectangle obds = getBounds();
				setBounds(obds.x, 0, obds.height, 600);
			}
		}
		if (state == NORMAL) {
			setBounds(normalBounds);
		}
		invalidate();
		validate();
	}
	
	public int getDesktopWidth() {
		return desktopWidth;
	}

	public void setDesktopWidth(int desktopWidth) {
		this.desktopWidth = desktopWidth;
	}

	public int getDesktopHeight() {
		return desktopHeight;
	}

	public void setDesktopHeight(int desktopHeight) {
		this.desktopHeight = desktopHeight;
	}

	public void setDesktopComponent(SceneGraphComponent windowRoot) {
		this.windowRoot = windowRoot;
		windowRoot.addChild(getSceneGraphComponent());
	}

	@Override
	public void dispose() {
		windowRoot.removeChild(getSceneGraphComponent());
		super.dispose();
	}
	
	public void setLayer(int layer) {
		this.layer = layer;
		if (windowComponent != null) {
			Rectangle bds = getBounds();
			double x = bds.x;
			double y = bds.y;
			MatrixBuilder.euclidean().translate(x,y,-layer).assignTo(windowComponent);
		}
	}
}
