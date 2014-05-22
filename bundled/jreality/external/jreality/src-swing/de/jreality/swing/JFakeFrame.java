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


package de.jreality.swing;

import java.awt.Color;
import java.awt.Component;
import java.awt.Graphics2D;
import java.awt.GraphicsConfiguration;
import java.awt.HeadlessException;
import java.awt.Toolkit;
import java.awt.event.ComponentEvent;
import java.awt.event.ComponentListener;
import java.awt.image.BufferedImage;

import javax.swing.JFrame;

import de.jreality.scene.Appearance;
import de.jreality.shader.CommonAttributes;
import de.jreality.shader.ImageData;
import de.jreality.shader.Texture2D;
import de.jreality.shader.TextureUtility;

public class JFakeFrame extends JFrame {
    private static final long serialVersionUID = 3258688793266958393L;
    MouseEventTool tool;
    BufferedImage bufferedImage;
    private Graphics2D graphics;

    Component current = null;
    Appearance appearance;
    private Texture2D tex;
    private String praefix = "polygonShader";

    private boolean mute=false; // if true the image is not updated
    
    public JFakeFrame() throws HeadlessException {
        super();
        init();
    }
    
    public JFakeFrame(GraphicsConfiguration gc) {
        super(gc);
        init();
    }

    public JFakeFrame(String title) throws HeadlessException {
        super(title);
        init();
    }

    public JFakeFrame(String title, GraphicsConfiguration gc) {
        super(title, gc);
        init();
    }

    protected void init() {
        
        tool = new MouseEventTool(this);
              
              appearance = new Appearance();
              appearance.setAttribute(CommonAttributes.DIFFUSE_COLOR,Color.WHITE);
              appearance.setAttribute(CommonAttributes.VERTEX_DRAW, false);
              appearance.setAttribute(CommonAttributes.EDGE_DRAW, false);
              appearance.setAttribute(CommonAttributes.TUBES_DRAW, false);
              
    }
    public void addNotify() {
        super.addNotify();
        ((FakeFramePeer)getPeer()).setRepaintAction(new Runnable() {

            public void run() {
                fire();
            }
            
        });

        //fire();
    }
    private void fire() {
      if (isMute()) return;
        FakeFramePeer peer = (FakeFramePeer)getPeer();
        if(peer != null) {
            bufferedImage = peer.getRootImage();
            graphics = bufferedImage.createGraphics();
            if (tool != null) tool.setSize(getWidth(),getHeight());
            if(graphics != null) {
                graphics.setColor(getBackground());
                graphics.fillRect(0, 0, getWidth(),getHeight());
                paint(graphics);
//                printAll(graphics);
                ImageData img = new de.jreality.shader.ImageData(bufferedImage);
                if(appearance != null) {
//                    System.err.print("set...");
                    if(tex == null) {
                        tex = TextureUtility.createTexture(appearance, praefix ,img);
                        tex.setMipmapMode(false);
                        tex.setMinFilter(Texture2D.GL_NEAREST);
                        tex.setMagFilter(Texture2D.GL_NEAREST);
                    }
                    else tex.setImage(img);
//                    System.err.println(". texture "+bufferedImage.getWidth());
                }
            }
        }
    }
    
    public MouseEventTool getTool() {
        return tool;
    }

    public Appearance getAppearance() {
        return appearance;
    }
    
    public Toolkit getToolkit() {
        return FakeToolKit.getDefaultToolkit();
    }

    /**
     * When the frame is muted, then the texture image is not
     * updated.
     * @return the mute state of this frame.
     */
    public boolean isMute() {
      return mute;
    }
    
    /**
     * When the frame is muted, then the texture image is not
     * updated.
     * 
     * @param mute set the mute state of this frame.
     */
    public void setMute(boolean mute) {
      this.mute = mute;
      fire();
    }
    
    
    protected synchronized void fireComponentResized() {
    	ComponentEvent ce = new ComponentEvent(this, 0);
    	for (ComponentListener l : getComponentListeners()) {
    		l.componentResized(ce);
    	}
    }
    
}
