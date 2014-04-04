package de.jreality.tutorial.app;

import java.awt.Color;
import java.awt.Component;
import java.awt.event.KeyAdapter;
import java.awt.event.KeyEvent;
import java.io.IOException;

import de.jreality.geometry.Primitives;
import de.jreality.plugin.JRViewer;
import de.jreality.scene.Appearance;
import de.jreality.scene.Viewer;
import de.jreality.shader.CommonAttributes;
import de.jreality.shader.ImageData;
import de.jreality.shader.TextureUtility;
import de.jreality.util.Input;

public class BackgroundExample {

	    static int showImage = 2;
	    static Appearance rootApp;
	     static ImageData id = null;
	   public static void main(String[] args) {
	      Viewer viewer = JRViewer.display(Primitives.coloredCube());
	      JRViewer view = JRViewer.getLastJRViewer();
	      try {
	         id = ImageData.load(Input.getInput("textures/grid.jpeg"));
	      } catch (IOException e) {
	         e.printStackTrace();
	      }
	      rootApp = view.getViewer().getSceneRoot().getAppearance();
	      updateBackground();
			Component comp = (Component) viewer.getViewingComponent();
			comp.addKeyListener(new KeyAdapter() {
	 				public void keyPressed(KeyEvent e)	{ 
						switch(e.getKeyCode())	{
							
						case KeyEvent.VK_H:
							System.err.println("	1: toggle background image");
							break;
			
						case KeyEvent.VK_1:
							showImage++;
							showImage = showImage % 3;
							updateBackground();
							break;


					}
			
					}
				});


	   }
	   static Color[] cornerColors = {Color.RED, Color.BLUE, Color.GREEN, Color.YELLOW};
	   private static void updateBackground()	{
		   if (showImage == 0){  // mono color
			   rootApp.setAttribute(CommonAttributes.BACKGROUND_TEXTURE2D, Appearance.INHERITED);
			   rootApp.setAttribute(CommonAttributes.BACKGROUND_COLOR, Color.RED);
			   rootApp.setAttribute(CommonAttributes.BACKGROUND_COLORS, Appearance.INHERITED);
		   } else if (showImage == 1){	// gradient based on 4 corners
			   rootApp.setAttribute(CommonAttributes.BACKGROUND_TEXTURE2D, Appearance.INHERITED);
			   rootApp.setAttribute(CommonAttributes.BACKGROUND_COLOR, Appearance.INHERITED);
			   rootApp.setAttribute(CommonAttributes.BACKGROUND_COLORS, cornerColors);			   
		   }
		   else if (showImage == 2)	{
			      // either
//			      RootAppearance ra = ShaderUtility.createRootAppearance(rootApp);
//			      ra.createBackgroundTexture2D().setImage(id)    
			      // or
			   TextureUtility.setBackgroundTexture(rootApp, id);
			   rootApp.setAttribute(CommonAttributes.BACKGROUND_COLOR, Appearance.INHERITED);
			   rootApp.setAttribute(CommonAttributes.BACKGROUND_COLORS, Appearance.INHERITED);
		   }
	   }
}
