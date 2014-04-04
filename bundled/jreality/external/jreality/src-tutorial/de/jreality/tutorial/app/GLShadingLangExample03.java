package de.jreality.tutorial.app;

import static de.jreality.shader.CommonAttributes.POLYGON_SHADER;

import java.awt.Color;
import java.io.IOException;

import de.jreality.geometry.SphereUtility;
import de.jreality.math.MatrixBuilder;
import de.jreality.plugin.JRViewer;
import de.jreality.scene.Appearance;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.shader.DefaultGeometryShader;
import de.jreality.shader.DefaultPolygonShader;
import de.jreality.shader.GlslProgram;
import de.jreality.shader.ImageData;
import de.jreality.shader.ShaderUtility;
import de.jreality.shader.Texture2D;
import de.jreality.shader.TextureUtility;
import de.jreality.tutorial.util.SimpleTextureFactory;
import de.jreality.tutorial.util.SimpleTextureFactory.TextureType;
import de.jreality.util.Input;
import de.jreality.util.SceneGraphUtility;

/**
 * A simple example of using an OpenGL shading language shader in a jReality scene graph.
 * @author Charles Gunn
 *
 */
public class GLShadingLangExample03 {

	public static void main(String[] args)	{
		SceneGraphComponent world = SceneGraphUtility.createFullSceneGraphComponent("world");
		Appearance ap = world.getAppearance();
		DefaultGeometryShader dgs = (DefaultGeometryShader) 
   			ShaderUtility.createDefaultGeometryShader(ap, true);
		dgs.setShowLines(false);
		dgs.setShowPoints(false);
//		dgs.createPolygonShader("glsl");
		dgs.createPolygonShader("default");
		DefaultPolygonShader dps = (DefaultPolygonShader) dgs.getPolygonShader();
		dps.setDiffuseColor(Color.white);
//		ap.setAttribute("useVertexArrays", false);
		ap.setAttribute("useGLSL", true);
		ap.setAttribute("transparencyEnabled", true);
		GlslProgram brickProg = null;		
		world.setGeometry(SphereUtility.sphericalPatch(0, 0, 90, 90, 30, 30, 1)); 
		MatrixBuilder.euclidean().rotateY(-Math.PI/2).assignTo(world);
		SimpleTextureFactory stf = new SimpleTextureFactory();
		stf.setType(TextureType.GRADIENT);
		stf.update();
		ImageData id =  stf.getImageData();
		Texture2D tex = TextureUtility.createTexture(ap, POLYGON_SHADER, 0, id);
		// rotate this texture by 90 degrees
		tex.setTextureMatrix(MatrixBuilder.euclidean().scale(4).rotateZ(Math.PI/2).getMatrix());
//		tex.setApplyMode(Texture2D.GL_DECAL);

		stf.setColor(0, Color.red);
		stf.update();
		id = stf.getImageData();
		Texture2D gradTexture = (Texture2D) TextureUtility.createTexture(ap, POLYGON_SHADER, 1, id); 
		gradTexture.setTextureMatrix(MatrixBuilder.euclidean().scale(3).getMatrix());
//		gradTexture.setApplyMode(Texture2D.GL_COMBINE);
		// sampler.frag:
//		uniform sampler2D  sampler;
//		uniform sampler2D sampler2;
//		uniform float BlendFactor;
//		void main(void)
//		{
//		    vec4 currentSample = texture2D(sampler,gl_TexCoord[0].st); 
//		    vec4 currentSample2 = texture2D(sampler2,gl_TexCoord[1].st);
//		    float alpha = BlendFactor * currentSample2.a; 
//		    gl_FragColor.rgb = mix(currentSample.rgb, currentSample2.rgb, alpha); //( currentSample.rgb * (1.0-alpha) + currentSample2.rgb *alpha); 
//		    gl_FragColor.a = 1.0;
//		}
		try {
			brickProg = new GlslProgram(ap, "polygonShader",   
					null,
					Input.getInput("de/jreality/jogl/shader/resources/sampler.frag")
			    );
		} catch (IOException e) {
			e.printStackTrace();
		}
		brickProg.setUniform("sampler",0);
		brickProg.setUniform("sampler2",1);	
		// a value of 1 will ue only the second texture, while 0 gives the first texture
		brickProg.setUniform("BlendFactor", 0.8);
		JRViewer.display(world);
//		CameraUtility.encompass(va.getCurrentViewer());
	}
}
