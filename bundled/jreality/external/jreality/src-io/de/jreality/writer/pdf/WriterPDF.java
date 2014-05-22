package de.jreality.writer.pdf;

import static com.itextpdf.text.pdf.PdfBoolean.PDFFALSE;
import static com.itextpdf.text.pdf.PdfBoolean.PDFTRUE;
import static de.jreality.util.SceneGraphUtility.getPathsBetween;

import java.awt.Color;
import java.awt.Dimension;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.LineNumberReader;
import java.io.OutputStream;
import java.io.Writer;
import java.util.ArrayList;
import java.util.List;

import com.itextpdf.text.Document;
import com.itextpdf.text.DocumentException;
import com.itextpdf.text.Rectangle;
import com.itextpdf.text.pdf.PdfAnnotation;
import com.itextpdf.text.pdf.PdfAppearance;
import com.itextpdf.text.pdf.PdfArray;
import com.itextpdf.text.pdf.PdfBoolean;
import com.itextpdf.text.pdf.PdfDictionary;
import com.itextpdf.text.pdf.PdfEncodings;
import com.itextpdf.text.pdf.PdfIndirectReference;
import com.itextpdf.text.pdf.PdfName;
import com.itextpdf.text.pdf.PdfStream;
import com.itextpdf.text.pdf.PdfString;
import com.itextpdf.text.pdf.PdfWriter;

import de.jreality.io.JrScene;
import de.jreality.reader.ReaderJRS;
import de.jreality.scene.Appearance;
import de.jreality.scene.Camera;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SceneGraphNode;
import de.jreality.scene.SceneGraphPath;
import de.jreality.util.Input;
import de.jreality.util.SceneGraphUtility;
import de.jreality.writer.SceneWriter;
import de.jreality.writer.u3d.U3DSceneUtility;
import de.jreality.writer.u3d.WriterU3D;

public class WriterPDF implements SceneWriter {

    public static final String 
	    PDF_NAME_3D = "3D",
	    PDF_NAME_3DD = "3DD",
	    PDF_NAME_3DV = "3DV",
	    PDF_NAME_3DVIEW = "3DView",
	    PDF_NAME_C2W = "C2W",
	    PDF_NAME_IN = "IN",
	    PDF_NAME_MS = "MS",
	    PDF_NAME_U3D = "U3D",
	    PDF_NAME_U3DPATH = "U3DPath",
	    PDF_NAME_XN = "XN";
	
    
    public enum PDF3DTool {
    	MEASURE("Measure"),
    	PAN("Pan"),
    	ROTATE("Rotate"),
    	SPIN("Spin"),
    	WALK("Walk"),
    	ZOOM("Zoom");
    	
    	private String
    		name = "";
    	
    	private PDF3DTool(String name) {
    		this.name = name;
    	}
    	
    	public String getName() {
    		return name;
    	}
    	
    }
    
    public static enum PDF3DLightingScene {
    	FILE("file"),
    	NONE("none"),
    	WHITE("white"),
    	DAY("day"),
    	BRIGHT("bright"),
    	RGB("rgb"),
    	NIGHT("night"),
    	BLUE("blue"),
    	RED("red"),
    	CUBE("cube"),
    	CAD("cad"),
    	HEADLAMP("headlamp");
    	
    	private String
    		name = "";
    	
    	private PDF3DLightingScene(String name) {
    		this.name = name;
    	}
    	
    	public String getName() {
    		return name;
    	}
    	
    }
    
    public static enum PDF3DRenderMode {
    	DEFAULT("default"),
    	BOUNDING_BOX("bounding box"),
    	TRANSPARENT_BOUNDING_BOX("transparent bounding box"),
    	TRANSPARENT_BOUNDING_BOX_OUTLINE("transparent bounding box outline"),
    	VERTICES("vertices"),
    	SHADED_VERTICES("shaded vertices"),
    	WIREFRAME("wireframe"),
    	SHADED_WIREFRAME("shaded wireframe"),
    	SOLID("solid"),
    	TRANSPARENT("transparent"),
    	SHADED_SOLID_WIREFRAME("solid wireframe"),
    	TRANSPARENT_WIREFRAME("transparent wireframe"),
    	ILLUSTRATION("illustration"),
    	SOLID_OUTLINE("solid outline"),
    	SHADED_ILLUSTRATION("shaded illustration"),
    	HIDDEN_WIREFRAME("hidden wireframe");
    	
    	private String
    		name = "";
    	
    	private PDF3DRenderMode(String name) {
    		this.name = name;
    	}
    	
    	public String getName() {
    		return name;
    	}
    	
    }
    
    
    public static enum PDF3DGridMode {
    	GRID_MODE_OFF("off"),
    	GRID_MODE_SOLID("solid"),
    	GRID_MODE_TRANSPARENT("transparent"),
    	GRID_MODE_WIRE("wire");
    	
    	private String
    		name = "";
    	
    	private PDF3DGridMode(String name) {
    		this.name = name;
    	}
    	
    	public String getName() {
    		return name;
    	}
    	
    }
    
    
    private Dimension
    	size = new Dimension(800, 600);
    private PDF3DTool
    	tool = PDF3DTool.ROTATE;
    private PDF3DLightingScene
    	lighting = PDF3DLightingScene.DAY;
    private PDF3DRenderMode
    	renderMode = PDF3DRenderMode.SOLID;
    private PDF3DGridMode
    	gridMode = PDF3DGridMode.GRID_MODE_OFF;
    private boolean
    	showInventory = false,
    	showGrid = false,
    	showAxes = false;
    private File
    	userScriptFile = null;
    private static final String
    	defaultScript = getJSScript("defaultPrefs.js");
    
	/**
	 * Exports a given {@link SceneGraphNode} into a PDF document. 
	 * @param c the scene graph node to export
	 * @param out the output stream to export the data to
	 */
	public void write(SceneGraphNode c, OutputStream out) throws IOException {
		SceneGraphComponent root = null;
		if (c instanceof SceneGraphComponent) root = (SceneGraphComponent) c;
		else {
			root = new SceneGraphComponent();
			SceneGraphUtility.addChildNode(root, c);
		}
		JrScene scene = new JrScene(root);
		writeScene(scene, out);
	}

	
	private static float[] toU3DMatrix(double[] M) {
		float[] R = new float[12];
		for (int i = 0; i < 4; i++) {
			for (int j = 0; j < 3; j++) {
				R[i * 3 + j] = (float)M[j * 4 + i];
			}
		}
		return R;
	}
	
	private String colorToString(Color c) {
		String cs = "";
		cs += c.getRed() / 255.0 + ", ";
		cs += c.getGreen() / 255.0 + ", ";
		cs += c.getBlue() / 255.0;
		return cs;
	}
	
	/**
	 * Exports a given {@link JrScene} into a PDF document. 
	 * @param scene the scene to export
	 * @param out the output stream to export the data to
	 */
	public void writeScene(JrScene scene, OutputStream out) throws IOException {
		// Write U3D data to temporary file
		File u3dTmp = File.createTempFile("jralityPDFExport", "u3d");
		FileOutputStream u3dout = new FileOutputStream(u3dTmp); 
		WriterU3D.write(scene, u3dout);

		List<SceneGraphComponent> cameraNodes = U3DSceneUtility.getViewNodes(scene);
		List<SceneGraphPath> camPaths = new ArrayList<SceneGraphPath>();
		for (SceneGraphComponent c : cameraNodes) {
			camPaths.addAll(getPathsBetween(scene.getSceneRoot(), c));
		}
		// Create PDF
		Rectangle pageSize = new Rectangle(size.width, size.height);
		Document doc = new Document(pageSize);
		try {
			PdfWriter wr = PdfWriter.getInstance(doc, out);
			doc.open();			
			
			String script = getSceneScript(scene);
			PdfStream oni = new PdfStream(PdfEncodings.convertToBytes(script, null));
            oni.flateCompress();
            PdfIndirectReference initScriptRef = wr.addToBody(oni).getIndirectReference();

            ArrayList<PdfIndirectReference> viewList = new ArrayList<PdfIndirectReference>(camPaths.size());
            for (SceneGraphPath path : camPaths) {
            	SceneGraphComponent c = path.getLastComponent();
            	Camera cam = c.getCamera();
            	float[] T1 = toU3DMatrix(path.getMatrix(null));
            	T1 = new float[]{0,0,0, 0,0,0, 0,0,0, 0,0,0};
	            PdfDictionary viewDict = new PdfDictionary(new PdfName(PDF_NAME_3DVIEW));
	            viewDict.put(new PdfName(PDF_NAME_MS), new PdfString("M"));viewDict.put(new PdfName(PDF_NAME_MS), new PdfString("M"));
//	            viewDict.put(PdfName.CO, new PdfNumber(10.0f));
	            viewDict.put(new PdfName("C2W"), new PdfArray(T1));
	            viewDict.put(new PdfName(PDF_NAME_XN), new PdfString(cam.getName()));
	            PdfIndirectReference ref = wr.addToBody(viewDict).getIndirectReference(); // Write view dictionary, get reference
	            viewList.add(ref);
            }
            
            PdfStream stream = new PdfStream(new FileInputStream(u3dTmp), wr);
			stream.put(new PdfName("OnInstantiate"), initScriptRef);
			stream.put(PdfName.TYPE, new PdfName(PDF_NAME_3D)); // Mandatory keys
			stream.put(PdfName.SUBTYPE, new PdfName(PDF_NAME_U3D));
//			stream.put(new PdfName("VA"), new PdfArray(viewList));
//			stream.put(new PdfName("VD"), new PdfNumber(0));
			stream.flateCompress();
			PdfIndirectReference u3dStreamRef = wr.addToBody(stream).getIndirectReference(); // Write stream contents, get reference to stream object, write actual stream length
			stream.writeLength();

            PdfDictionary activationDict = new PdfDictionary();
            activationDict.put(PdfName.A, new PdfName("PO"));
            activationDict.put(new PdfName("DIS"), PdfName.I);
            
            PdfAppearance ap = PdfAppearance.createAppearance(wr, pageSize.getRight() - pageSize.getLeft(), pageSize.getTop() - pageSize.getBottom());
            ap.setBoundingBox(pageSize);
            
            PdfAnnotation annot = new PdfAnnotation(wr, pageSize);
            annot.put(PdfName.CONTENTS, new PdfString("3D Model"));
            annot.put(PdfName.SUBTYPE, new PdfName(PDF_NAME_3D)); // Mandatory keys
            annot.put(PdfName.TYPE, PdfName.ANNOT);
            annot.put(new PdfName(PDF_NAME_3DD), u3dStreamRef); // Reference to stream object
            PdfBoolean value3DI = showInventory ? PDFTRUE : PDFFALSE;
            annot.put(new PdfName("3DI"), value3DI);
//            annot.put(new PdfName("3DV"), new PdfName("F")); // First view is the default one
            annot.setAppearance(PdfAnnotation.APPEARANCE_NORMAL, ap); // Assign appearance and page
            annot.put(new PdfName("3DA"), activationDict);
            annot.setPage(1);
            
 
            // Actually write annotation
            wr.addAnnotation(annot);
			
			doc.close();
		} catch (DocumentException e) {
			e.printStackTrace();
		}
	}
	
	
	private String getSceneScript(JrScene scene) {
		if (userScriptFile != null) {
			FileInputStream fin;
			try {
				fin = new FileInputStream(userScriptFile);
			} catch (FileNotFoundException e) {
				e.printStackTrace();
				return "";
			}
			return getJSScript(fin);
		} else {
			String script = defaultScript;
			script = script.replace("##cam##", "DefaultView");
			Appearance rootApp = scene.getSceneRoot().getAppearance();
			Color cUpper = Color.WHITE;
			Color cLower = Color.WHITE; 
			if (rootApp != null) {
				if (rootApp.getAttribute("backgroundColors") instanceof Color[]) {
					Color[] colors = (Color[])rootApp.getAttribute("backgroundColors");
					cUpper = colors[0];
					cLower = colors[2];
				} else if (rootApp.getAttribute("backgroundColor") instanceof Color) {
					Color color = (Color)rootApp.getAttribute("backgroundColor");
					cUpper = color;
					cLower = color;
				}
			}
			script = script.replace("##backgroundColorUpper##", colorToString(cUpper));
			script = script.replace("##backgroundColorLower##", colorToString(cLower));
			script = script.replace("##tool##", tool.getName());
			script = script.replace("##lighting##", lighting.getName());
			script = script.replace("##render_mode##", renderMode.getName());
			script = script.replace("##grid_mode##", gridMode.getName());
			script = script.replace("##show_axes##", ((Boolean)showAxes).toString());
			script = script.replace("##show_grid##", ((Boolean)showGrid).toString());
			return script;
		}
	}
	
	

	/**
	 * This method cannot be used for PDF exporting. 
	 * It always throws an {@link UnsupportedOperationException}.
	 * @param scene unused
	 * @param out unused
	 */
	@Deprecated
	public void writeScene(JrScene scene, Writer out) throws IOException, UnsupportedOperationException {
		throw new UnsupportedOperationException("PDF is a binary file format");
	}

	
	/**
	 * Exports a given {@link JrScene} into a PDF document. 
	 * @param scene the scene to export
	 * @param out the output stream to export the data to
	 */
	public static void write(JrScene scene, OutputStream out) throws IOException {
		WriterU3D writer = new WriterU3D();
		writer.writeScene(scene, out);
	}
	

	private static String getJSScript(String resourceName) {
		InputStream in = WriterPDF.class.getResourceAsStream(resourceName);
		return getJSScript(in);
	}
	
	
	private static String getJSScript(InputStream in) {
		StringBuffer result = new StringBuffer();
		InputStreamReader inReader = new InputStreamReader(in);
		LineNumberReader lineIn = new LineNumberReader(inReader);
		String line = null;
		try {
			while ((line = lineIn.readLine()) != null) {
				result.append(line + "\n");
			}
		} catch (IOException e) {
			e.printStackTrace();
		}
		return result.toString();
	}

	
	public void setTool(PDF3DTool tool) {
		this.tool = tool;
	}
	
	public void setSize(Dimension size) {
		this.size = size;
	}
	
	public void setLighting(PDF3DLightingScene lighting) {
		this.lighting = lighting;
	}
	
	public void setRenderMode(PDF3DRenderMode renderMode) {
		this.renderMode = renderMode;
	}
	
	public void setGridMode(PDF3DGridMode gridMode) {
		this.gridMode = gridMode;
	}
	
	public void setShowAxes(boolean showAxes) {
		this.showAxes = showAxes;
	}
	
	public void setShowGrid(boolean showGrid) {
		this.showGrid = showGrid;
	}
	
	public void setUserScriptFile(File userScriptFile) {
		this.userScriptFile = userScriptFile;
	}
	
	public void setShowInventory(boolean showInventory) {
		this.showInventory = showInventory;
	}
	
	public static void main(String[] args) {
		ReaderJRS reader = new ReaderJRS();
		SceneGraphComponent root = null;
		try {
			root = reader.read(Input.getInput(WriterPDF.class.getResource("test.jrs")));
		} catch (IOException e) {
			e.printStackTrace();
			return;
		}
		WriterPDF writer = new WriterPDF();
		FileOutputStream out;
		try {
			out = new FileOutputStream("test.pdf");
			writer.write(root, out);
		} catch (FileNotFoundException e) {
			e.printStackTrace();
		} catch (IOException e) {
			e.printStackTrace();
		}
	}
	
	
}
