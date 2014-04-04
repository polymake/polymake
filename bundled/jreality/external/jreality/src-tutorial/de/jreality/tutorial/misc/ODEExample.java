package de.jreality.tutorial.misc;

import java.awt.Color;

import de.jreality.geometry.IndexedLineSetUtility;
import de.jreality.plugin.JRViewer;
import de.jreality.scene.Appearance;
import de.jreality.scene.IndexedLineSet;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.Viewer;
import de.jreality.shader.CommonAttributes;
import de.jreality.util.SceneGraphUtility;
import de.jtem.numericalMethods.calculus.odeSolving.Extrap;
import de.jtem.numericalMethods.calculus.odeSolving.ODE;

public class ODEExample {

	int numCurves = 20,
			timeSteps = 100;
	double scale = 1.05;
	public static void main(String[] args) {
		ODEExample ode = new ODEExample();
		ode.doIt();
	}
	
	double angle = .9*Math.PI/2, c = Math.cos(angle), s = Math.sin(angle);
	public void doIt()	{
		SceneGraphComponent world = SceneGraphUtility.createFullSceneGraphComponent("world");
		final Extrap extrap = new Extrap(2);
		extrap.setAbsTol(10E-10);
		final ODE ode = new ODE() {
			
			public int getNumberOfEquations() {
				return 2;
			}
			// define the vector field y as a function of position x
			// in this case, rotate the position by the angle above to obtain the tangent vector
			// solution curves are logarithmic spirals
			public void eval(double t, double[] x, double[] y) {
				y[0] = c*x[0]-s*x[1];
				y[1] = s*x[0]+c*x[1];
			}
		};
		double[] solution =  new double[2];
		double deltaT = .05;
		for (int j = 0; j<numCurves; ++j)	{
			for (int k = 0; k<2; ++k)	{  // forward and backward time
				double[][] verts = new double[timeSteps][];
				double cangle = 2*Math.PI*j/((double) numCurves);
				solution[0] = Math.cos(cangle);
				solution[1] = Math.sin(cangle);
				double direction = (k == 0) ? 1 : -1;
				for (int i = 0; i<timeSteps; ++i)	{
					verts[i] = new double[]{solution[0], solution[1], 0};
					// solve the ode with initial condition solution from time 0 to time direction*deltaT
					// and put the solution back into the same array solution.
					extrap.odex(ode, solution, 0, direction*deltaT);	
					// this and the previous are equivalent as long as the eval() method ignores the t parameter.
//					extrap.odex(ode, solution, direction * i*deltaT, direction*(i+1)*deltaT);	
				}
				IndexedLineSet curve = IndexedLineSetUtility.createCurveFromPoints(verts, false);
				SceneGraphComponent child = new SceneGraphComponent("child"+j);
				world.addChild(child);
				child.setGeometry(curve);				
			}
		}
		Appearance ap = world.getAppearance();
		ap.setAttribute(CommonAttributes.LINE_SHADER+"."+CommonAttributes.DIFFUSE_COLOR, Color.white);
		ap.setAttribute(CommonAttributes.LINE_SHADER+"."+CommonAttributes.TUBE_RADIUS, .01);
		ap.setAttribute(CommonAttributes.VERTEX_DRAW, false);
		Viewer v = JRViewer.display(world);
		v.getSceneRoot().getAppearance().setAttribute("backgroundColor", Color.black);
	}
}
