package de.jreality.tutorial.tool;

import java.io.IOException;

import de.jreality.geometry.Primitives;
import de.jreality.math.MatrixBuilder;
import de.jreality.plugin.JRViewer;
import de.jreality.plugin.JRViewer.ContentType;
import de.jreality.reader.Readers;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.tool.AbstractTool;
import de.jreality.scene.tool.InputSlot;
import de.jreality.scene.tool.Tool;
import de.jreality.scene.tool.ToolContext;
import de.jreality.shader.CommonAttributes;
import de.jreality.util.Input;

public class AnimationExample {
	
	SceneGraphComponent centerCmp = new SceneGraphComponent("center cmp");
	SceneGraphComponent onEllipseCmp = new SceneGraphComponent("on ellipse");

	// half-axes of the ellipse
	protected double a = 15, b = 7.5;
	
	// angular velocity along the ellipse
	final double omega = 1;
	
	// the point on the ellipse
	double[] gamma = new double[3];
	// the tangent vector at the current ellipse point
	double[] gammaDot = new double[3];
	// the tangent vector at t=0
	double[] gammaDot0 = new double[]{0,0,-1};

	// a simple tool (always active) that is triggered by SYSTEM_TIME
	Tool animator = new AbstractTool() {
		long startTime=-1;
		{
			addCurrentSlot(InputSlot.SYSTEM_TIME, "animation trigger");
		}
		@Override
		public void perform(ToolContext tc) {
			
			if (startTime==-1) startTime = tc.getTime();
			// compute time in seconds since startup;
			double t = 0.001*(tc.getTime()-startTime);

			// compute the current point and tangent of the ellipse
			double angle = t*omega;
			gamma[0] = a*Math.cos(angle);
			gamma[2] = -b*Math.sin(angle);
			
			gammaDot[0] = -omega*a*Math.sin(t*omega);
			gammaDot[2] = -omega*b*Math.cos(t*omega);
			
			// obtain the component where the tool is attached
			// this component is moving along the ellipse
			SceneGraphComponent myCmp = tc.getRootToToolComponent().getLastComponent();
			
			// compute the corresponding transformation and apply it to myCmp:
			MatrixBuilder.euclidean()
				.translate(gamma)
				.rotateFromTo(gammaDot0, gammaDot)
				.assignTo(myCmp);

		}
	};

	
	public AnimationExample() {
		try {
			SceneGraphComponent bear = Readers.read(Input.getInput("jrs/baer.jrs"));
			// the bear is a huge geometry and we cannot afford drawing
			// edges and vertices while having real time audio...
			bear.getAppearance().setAttribute(CommonAttributes.VERTEX_DRAW, false);
			bear.getAppearance().setAttribute(CommonAttributes.EDGE_DRAW, false);
			MatrixBuilder.euclidean().rotateX(-Math.PI/2).rotateZ(Math.PI).translate(0,0,2).scale(0.002).assignTo(bear);
			onEllipseCmp.addChild(bear);
		} catch (IOException ioe) {
			// no bear available
			SceneGraphComponent icoCmp = new SceneGraphComponent();
			MatrixBuilder.euclidean().translate(0,1.7,0).assignTo(icoCmp);
			icoCmp.setGeometry(Primitives.box(1, 2, 5, true));
			onEllipseCmp.addChild(icoCmp);
		}
		centerCmp.addChild(onEllipseCmp);
		
		onEllipseCmp.addTool(animator);
		
	}

	protected SceneGraphComponent getCenterComponent() {
		return centerCmp;
	}
	
	protected SceneGraphComponent getMovingComponent() {
		return onEllipseCmp;
	}
		
	public static void main(String[] args) {
		AnimationExample example = new AnimationExample();

		// create a VR viewer that does no alignment to the content:
		JRViewer v = new JRViewer();
		v.addBasicUI();
		v.addVRSupport();
		v.addContentSupport(ContentType.Raw);
		
		// set the center component of the ellipse as content:
		v.setContent(example.getCenterComponent());
		
		// startup the viewer:
		v.startup();

	}

}
