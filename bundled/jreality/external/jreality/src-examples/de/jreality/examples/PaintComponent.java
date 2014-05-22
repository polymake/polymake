/**
 *
 * This file is part of jReality. jReality is open source software, made
 * available under a BSD license:
 *
 * Copyright (c) 2003-2006, jReality Group: Charles Gunn, Tim Hoffmann, Markus
 * Schmies, Steffen Weissmann.
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * - Neither the name of jReality nor the names of its contributors nor the
 *   names of their associated organizations may be used to endorse or promote
 *   products derived from this software without specific prior written
 *   permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */


package de.jreality.examples;

import java.awt.BasicStroke;
import java.awt.Color;
import java.awt.Dimension;
import java.awt.Graphics;
import java.awt.Graphics2D;
import java.awt.RenderingHints;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.MouseEvent;
import java.awt.event.MouseListener;
import java.awt.event.MouseMotionListener;
import java.awt.image.BufferedImage;

import javax.swing.JButton;
import javax.swing.JPanel;

import de.jreality.math.MatrixBuilder;
import de.jreality.plugin.JRViewer;
import de.jreality.plugin.content.ContentAppearance;
import de.jreality.plugin.content.ContentLoader;
import de.jreality.plugin.content.ContentTools;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.Transformation;
import de.jreality.scene.pick.AABBTree;
import de.jreality.swing.JFakeFrame;
import de.jreality.util.PickUtility;

public class PaintComponent extends JPanel implements MouseListener, MouseMotionListener,ActionListener {
    BufferedImage myoff;
    int xold, yold;
    private Graphics2D graphics;
    private JButton button = new JButton("Clear");
    public PaintComponent() {
        super();
        add(button);
        button.addActionListener(this);
        addMouseListener(this);
        addMouseMotionListener(this);
    }

    public void paintComponent(Graphics g) {
        super.paintComponent(g);
       if(myoff!= null) g.drawImage(myoff,0,0,null);
    }

    public void mouseDragged(MouseEvent e) {
        //System.out.println("cp m dragged "+e);
        if(myoff == null) {
            clearMe();
        }
        int x = e.getX();
        int y = e.getY();
        graphics.drawLine(xold,yold,x,y);
        xold=x;
        yold = y;
        repaint();
        //invalidate();
    }
    
    private void clearMe() {
        if(myoff == null) {
            myoff = new BufferedImage(getWidth(),getHeight(),BufferedImage.TYPE_INT_ARGB);
            graphics = myoff.createGraphics();
        }
        graphics.setColor(Color.WHITE);
        graphics.fillRect(0,0,getWidth()-1,getHeight()-1);
        graphics.setColor(Color.RED);
        graphics.setRenderingHint(RenderingHints.KEY_ANTIALIASING,RenderingHints.VALUE_ANTIALIAS_ON);
        graphics.setStroke(new BasicStroke(1.1f));
        for(int i = 0; i<225;i+=20) {
            graphics.drawLine(i,0,i,255);
            graphics.drawLine(0,i,255,i);
        }
        graphics.setColor(Color.BLACK);        
        graphics.setStroke(new BasicStroke(2.2f));
        repaint();
        //invalidate();
    }

    public void validate() {
        if(myoff == null || this.getWidth() != myoff.getWidth() || this.getHeight() != myoff.getHeight()){
            myoff = null;
            clearMe();
        }
        
        super.validate();
    }

    
    public void mouseMoved(MouseEvent e) {
        //System.out.println("cp m moved");
    }

    public void mouseClicked(MouseEvent e) {
        //System.out.println("cp m clicked");
    }

    public void mouseEntered(MouseEvent e) {
        //System.out.println("cp m entered");
    }

    public void mouseExited(MouseEvent e) {
        //System.out.println("cp m exited "+e);
        //System.out.println("  --> "+e.getSource());
    }

    public void mousePressed(MouseEvent e) {
        //System.out.println("cp m pressed");
        xold = e.getX();
        yold = e.getY();
        
    }

    public void mouseReleased(MouseEvent e) {
        //System.out.println("cp m released");
    }

    public void actionPerformed(ActionEvent e) {
        clearMe();
    }

    public Dimension getPreferredSize() {
        return new Dimension(256,256);
    }

    
    public static void main(String[] args) {
      PaintComponent pc = new PaintComponent();

      CatenoidHelicoid catenoid = new CatenoidHelicoid(50);
      catenoid.setAlpha(Math.PI/2.-0.3);
      
      AABBTree aabb = AABBTree.construct(catenoid, 10);
      
      catenoid.setGeometryAttributes(PickUtility.AABB_TREE, aabb);
      
      SceneGraphComponent catComp= new SceneGraphComponent();
      
      //catComp.addChild(aabb.display());
      
      Transformation gt= new Transformation();
      catComp.setTransformation(gt);
      MatrixBuilder.euclidean().rotateX(Math.PI).assignTo(catComp);
      
      
      catComp.setGeometry(catenoid);
      
      //JRJComponent jrj = new JRJComponent();
      JFakeFrame jrj = new JFakeFrame("bla blubb");
      jrj.add(pc);
      jrj.pack();
      jrj.setVisible(true);
      
      catComp.addTool(jrj.getTool());
      
   System.out.print("setting appearance ");
      catComp.setAppearance(jrj.getAppearance());
      System.out.println("done");

		JRViewer v = new JRViewer();
		v.addBasicUI();
		v.setContent(catComp);
		v.registerPlugin(new ContentAppearance());
		v.registerPlugin(new ContentLoader());
		v.registerPlugin(new ContentTools());
		v.startup();
   }

}
