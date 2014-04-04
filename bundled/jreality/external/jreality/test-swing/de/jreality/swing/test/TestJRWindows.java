package de.jreality.swing.test;

import java.awt.Color;

import javax.swing.JButton;
import javax.swing.JCheckBox;
import javax.swing.JFrame;
import javax.swing.JPanel;
import javax.swing.JTextArea;

import de.jreality.examples.PaintComponent;
import de.jreality.geometry.Primitives;
import de.jreality.io.JrScene;
import de.jreality.io.JrSceneFactory;
import de.jreality.plugin.JRViewer;
import de.jreality.plugin.content.ContentAppearance;
import de.jreality.plugin.content.ContentLoader;
import de.jreality.plugin.content.ContentTools;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SceneGraphPath;
import de.jreality.swing.jrwindows.JRWindowManager;
import de.jreality.tools.RotateTool;

public class TestJRWindows {
  public static void main(String[] args) {
    
    JrScene scene = JrSceneFactory.getDefaultDesktopScene();
    //JrScene scene = JrSceneFactory.getDefaultPortalScene();
    
    SceneGraphComponent root = scene.getSceneRoot();
    SceneGraphPath cameraPath = scene.getPath("cameraPath");
    SceneGraphPath emptyPickPath = scene.getPath("emptyPickPath");
    SceneGraphPath avatarPath = scene.getPath("avatarPath");  
    
    SceneGraphComponent sgc=new SceneGraphComponent();
    sgc.addTool(new RotateTool());
    sgc.setGeometry(Primitives.cube());
    root.addChild(sgc);
    
    JRWindowManager wm=new JRWindowManager(avatarPath.getLastComponent());   
    
    JFrame frame2=wm.createFrame().getFrame();
    frame2.getContentPane().add(new JTextArea("testareatestareatestareatestarea \n testareatestareatestareatestarea \n testareatestareatestareatestarea \n testareatestareatestareatestareatestarea \n testareatestareatestareatestareatestarea \n testareatestareatestareatestareatestarea \n testareatestareatestareatestareatestarea",10,50));
//
//    
    JFrame frame3=wm.createFrame().getFrame();
    frame3.getContentPane().add("North",new JButton("test"));
    PaintComponent pc=new PaintComponent();
    pc.setSize(400,180);
    pc.validate();
    frame3.getContentPane().add(pc);

//  
    JFrame frame4=wm.createFrame().getFrame();
    //frame4.getContentPane().setSize(100,50);
//    JCheckBox checkBox=new JCheckBox();
    JPanel panel=new JPanel();
    //JButton b=new JButton();
    panel.setSize(100,100); 
    panel.repaint();
    panel.validate();
   
   // panel.add(b);
    
   
    panel.setBackground(Color.RED);
    
    //panel.validate();
    frame4.getContentPane().add(panel);

    //checkBox.validate();
    //frame4.getContentPane().validate();
    //frame4.validate();
   
    System.out.println("vor: panelSize = "+panel.getWidth()+" ,"+panel.getHeight());
    System.out.println("vor: frameSize = "+frame4.getWidth()+" ,"+frame4.getHeight());

    
    //frame4.getContentPane().add(new JCheckBox());
    //frame4.getContentPane().add("South",new JCheckBox());    
  
    JFrame frame5=wm.createFrame().getFrame();
    frame5.getContentPane().add("North",new JCheckBox());
    //frame5.getContentPane().add("South",new JCheckBox());    
    
    
//    JFrame frame6=wm.createFrame();
//    frame6.getContentPane().add("North",new JCheckBox());  
//  
//    JFrame frame7=wm.createFrame();
//    frame7.getContentPane().add(new JLabel("labellabellabel"));   
    
    //wm.pack(); 
    //wm.validate();
    
    frame2.pack();
    frame3.pack();
    frame4.pack();
    frame5.pack();

    frame2.show();
    frame3.show();
    frame4.show();
    frame5.show();
    
    System.out.println("nach: panelSize = "+panel.getWidth()+" ,"+panel.getHeight());
    System.out.println("nach: frameSize = "+frame4.getWidth()+" ,"+frame4.getHeight());
    JRViewer v = new JRViewer();
	v.addBasicUI();
	v.setContent(root);
	v.registerPlugin(new ContentAppearance());
	v.registerPlugin(new ContentLoader());
	v.registerPlugin(new ContentTools());
	v.startup();
  }
}
