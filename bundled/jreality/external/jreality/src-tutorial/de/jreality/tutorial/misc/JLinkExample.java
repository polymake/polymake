package de.jreality.tutorial.misc;

import static de.jreality.shader.CommonAttributes.BACKGROUND_COLOR;
import static de.jreality.shader.CommonAttributes.BACKGROUND_COLORS;

import java.awt.Color;
import java.awt.Component;
import java.awt.Dimension;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.text.Format;

import javax.swing.BorderFactory;
import javax.swing.Box;
import javax.swing.JComboBox;
import javax.swing.JPanel;
import javax.swing.SwingConstants;
import javax.swing.border.CompoundBorder;
import javax.swing.border.EmptyBorder;

import com.wolfram.jlink.*;

import de.jreality.geometry.FrameFieldType;
import de.jreality.geometry.QuadMeshFactory;
import de.jreality.scene.Appearance;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.shader.CommonAttributes;
import de.jreality.shader.DefaultGeometryShader;
import de.jreality.shader.DefaultLineShader;
import de.jreality.shader.DefaultPointShader;
import de.jreality.shader.DefaultPolygonShader;
import de.jreality.shader.ShaderUtility;
import de.jreality.tutorial.util.TextSlider;
import de.jreality.ui.viewerapp.ViewerApp;
import de.jreality.util.CameraUtility;
import de.jreality.util.SceneGraphUtility;

public class JLinkExample {

	static double r = .4, R = 1.0;
	static int n = 20, m = 40;
	static QuadMeshFactory qmf;
    static KernelLink ml = null;
    static String workspace = "/Users/gunn/Software/workspace/";
	static String mmaFile = "jreality/src-tutorial/de/jreality/tutorial/misc/torus.m";
	static String mmaKernel = "/Applications/Mathematica 5.0.app/Contents/MacOS/MathKernel";
    public static void main(String[] argv) {
    	// following shows the arguments needed on my machine: adapt 
    	// location of mathematica for your system
    	if (argv == null || argv.length == 0)	{
    		argv = new String[]{
    		"-linkmode",
    		"launch",
    		"-linkname",
    		"\""+mmaKernel+"\""+" -mathlink "  	};
    	}
        System.err.println("argv = ");
        for (int i = 0; i<argv.length; ++i)	{
        	System.err.println(argv[i]);
        }
        try {
            ml = MathLinkFactory.createKernelLink(argv);
            // Get rid of the initial InputNamePacket the kernel will send
            // when it is launched.
            ml.discardAnswer();
        } catch (MathLinkException e) {
            System.out.println("Fatal error opening link: " + e.getMessage());
            return;
        }
    	JLinkExample tjl = new JLinkExample();
    	tjl.doIt();
    }
    
    public void doIt() {

		// now connect to mathematica
		try {
        	// first, some simple examples
			ml.evaluate("2+2");
			ml.waitForAnswer();

			int result = ml.getInteger();
			System.out.println("2 + 2 = " + result);

			// Here's how to send the same input, but not as a string:
			ml.putFunction("EvaluatePacket", 1);
			ml.putFunction("Plus", 2);
			ml.put(3);
			ml.put(3);
			ml.endPacket();
			ml.waitForAnswer();
			result = ml.getInteger();
			System.out.println("3 + 3 = " + result);

			// If you want the result back as a string, use evaluateToInputForm
			// or evaluateToOutputForm. The second arg for either is the
			// requested page width for formatting the string. Pass 0 for
			// PageWidth->Infinity. These methods get the result in one
			// step--no need to call waitForAnswer.
			String strResult = ml.evaluateToOutputForm("4+4", 0);
			System.out.println("4 + 4 = " + strResult);

			// read in a mathematica package which contains the definition of a torus
            ml.evaluate("<<"+workspace+mmaFile); // "<<MyPackage.m");
			ml.discardAnswer();

			// the interesting part!
			updateTorusFromMma();

			SceneGraphComponent torussgc = SceneGraphUtility
					.createFullSceneGraphComponent("torus knot");
			setupAppearance(torussgc.getAppearance());
			torussgc.setGeometry(qmf.getIndexedFaceSet());
			ViewerApp va = ViewerApp.display(torussgc);
			CameraUtility.encompass(va.getViewer());
			Appearance rootAp = va.getCurrentViewer().getSceneRoot().getAppearance();
			rootAp.setAttribute(BACKGROUND_COLORS, Appearance.INHERITED);
			rootAp.setAttribute(BACKGROUND_COLOR, new Color(0,20,40));
			
			Component insp = getInspector();
			va.addAccessory(insp);
			va.setFirstAccessory(insp);
			va.setAttachNavigator(true);
			va.update();
			va.getFrame().setName("M'ma JLink tutorial");

        } catch (MathLinkException e) {
            System.out.println("MathLinkException occurred: " + e.getMessage());
        } 
    }

	private void updateTorusFromMma(){
		if (qmf == null)	{
	    	// set up the quad mesh factory
			qmf = new QuadMeshFactory();
			qmf.setClosedInUDirection(true);
			qmf.setClosedInVDirection(true);
			qmf.setGenerateEdgesFromFaces(true);
			qmf.setGenerateFaceNormals(true);
			qmf.setGenerateFaceNormals(true);
		}
		// write the expression we want to express using the format() method
		String command = String.format("torus[%g, %g, %d, %d]", r, R, m, n);
		System.err.println("command = "+command);
		try {
			ml.evaluate(command);
			ml.waitForAnswer();

			Expr expr = ml.getExpr();
			int[] dimn = expr.dimensions();
			if (dimn.length != 3)
				throw new IllegalStateException("wrong dimensions");
			int[] part = { 0, 0, 0 };
			// build up the quad mesh from the returned expression
			double[][][] mesh = new double[dimn[0]][dimn[1]][dimn[2]];
			for (int i = 0; i < dimn[0]; ++i) {
				part[0] = i + 1;
				for (int j = 0; j < dimn[1]; ++j) {
					part[1] = j + 1;
					for (int k = 0; k < dimn[2]; ++k) {
						part[2] = k + 1;
						mesh[i][j][k] = expr.part(part).asDouble();
					}
				}
			}
			qmf.setVLineCount(dimn[0]);
			qmf.setULineCount(dimn[1]);
			qmf.setVertexCoordinates(mesh);
			qmf.update();
		} catch (MathLinkException e) {
			e.printStackTrace();
		} catch (ExprFormatException e) {
			e.printStackTrace();
		}
	}
	
	private  Component getInspector() {
		Box container = Box.createVerticalBox();
		container.setBorder(new CompoundBorder(new EmptyBorder(5, 5, 5, 5),
				BorderFactory.createTitledBorder(BorderFactory
						.createEtchedBorder(), "Torus parameters")));

		final TextSlider.Double RSlider = new TextSlider.Double("R",
				SwingConstants.HORIZONTAL, 0.0, 2, R);
		RSlider.addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent e) {
				R = RSlider.getValue();
				updateTorusFromMma();
			}
		});
		container.add(RSlider);
		
		final TextSlider.Double rSlider = new TextSlider.Double("r",
				SwingConstants.HORIZONTAL, 0.0, 1, r);
		rSlider.addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent e) {
				r = rSlider.getValue();
				updateTorusFromMma();
			}
		});
		container.add(rSlider);
		
		final TextSlider rtSlider = new TextSlider.Integer("u count",
				SwingConstants.HORIZONTAL, 2, 100, n);
		rtSlider.addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent e) {
				n = rtSlider.getValue().intValue();
				updateTorusFromMma();
			}
		});
		container.add(rtSlider);

		final TextSlider mSlider = new TextSlider.Integer("v count",
				SwingConstants.HORIZONTAL, 2, 100, m);
		mSlider.addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent e) {
				m = mSlider.getValue().intValue();
				updateTorusFromMma();
			}
		});
		container.add(mSlider);


		container.setPreferredSize(new Dimension(300, 180));
		JPanel panel = new JPanel();
		panel.setName("Parameters");
		panel.add(container);
		panel.add(Box.createVerticalGlue());
		return panel;
	}

	private static void setupAppearance(Appearance ap) {
		DefaultGeometryShader dgs;
		DefaultLineShader dls;
		DefaultPointShader dpts;
		dgs = ShaderUtility.createDefaultGeometryShader(ap, true);
		dls = (DefaultLineShader) dgs.createLineShader("default");
		dls.setDiffuseColor(Color.yellow);
		dls.setTubeRadius(.015);
		dpts = (DefaultPointShader) dgs.createPointShader("default");
		dpts.setDiffuseColor(Color.red);
		dpts.setPointRadius(.03);
	}

}