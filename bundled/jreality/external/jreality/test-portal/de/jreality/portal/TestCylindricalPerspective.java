package de.jreality.portal;

import de.jreality.geometry.Primitives;
import de.jreality.math.MatrixBuilder;
import de.jreality.scene.Appearance;
import de.jreality.scene.Camera;
import de.jreality.scene.Geometry;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.shader.GlslProgram;
import de.jreality.ui.viewerapp.ViewerApp;
import de.jreality.util.Input;

public class TestCylindricalPerspective {

	static String vertexShader = "\n"
	+"void main(void) {\n"
	+"  gl_Position = ftransform(); \n"
	+"  gl_TexCoord[0]  = gl_MultiTexCoord0;  \n"
	+"  gl_TexCoord[1]  = gl_MultiTexCoord1;  \n"
	+"  gl_TexCoord[2]  = gl_MultiTexCoord2;  \n"
	+"  vec3 normal = normalize(gl_NormalMatrix * normalize(gl_Normal)); \n"
	+"  vec3 lightDir = normalize(vec3(1,1,1)); //gl_LightSource[0].position)); \n"
	+"  float NdotL = max(dot(normal, lightDir), dot(-normal, lightDir)); \n"
	+"  vec4 diffuse = gl_FrontMaterial.diffuse * vec4(1,1,1,1); //gl_LightSource[0].diffuse; \n"
	+"  gl_FrontColor =  NdotL * diffuse;\n"
	+"  gl_BackColor =  NdotL * diffuse;\n"
	+"\n"
	+"\n"
	+"}\n";
	
	public static void main(String[] args) throws Exception {
		System.out.println(2*4.068*0.3048);
		System.out.println(Math.atan2(.1, 1));
		System.out.println(Math.atan2(-.1, 1));
//		System.out.println(Math.PI/2);
		SceneGraphComponent cmp = new SceneGraphComponent();
		ViewerApp va = new ViewerApp(cmp);
		Appearance ra = va.getSceneRoot().getAppearance();
		
		Camera cam = (Camera) va.getViewer().getCameraPath().getLastElement();
		cam.setNear(0.1);
		cam.setFar(1000);
		cam.setStereo(true);
		cam.setOnAxis(false);
		ra.setAttribute("polygonShadername", "glsl");
		GlslProgram prog = new GlslProgram(ra, "polygonShader", Input.getInput("de/jreality/portal/cylStereoVertexShader.glsl"), null);
//		prog.setUniform("near", cam.getNear());
//		prog.setUniform("far", cam.getFar());
//		prog.setUniform("d", 1.24);
//		Rectangle2D vp = CameraUtility.getViewport(cam, 1.25);
//		prog.setUniform("cv", new double[]{vp.getMinX(), vp.getMaxX(), vp.getMinY(), vp.getMaxY()});
//		GlslProgram prog = new GlslProgram(ra, "polygonShader", vertexShader, null);
		
//		Geometry g = new CatenoidHelicoid(20);
//		Geometry g = new Sphere();
		Geometry g = Primitives.torus(4, 2, 100, 50);
		cmp.setGeometry(g);
		MatrixBuilder.euclidean().translate(0,0,-10).assignTo(cmp);
		va.update();
		va.display();
	}
	
}
