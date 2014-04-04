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

package de.jreality.softviewer;

import java.awt.AWTEvent;
import java.awt.Color;
import java.awt.Component;
import java.awt.Dimension;
import java.awt.EventQueue;
import java.awt.Graphics;
import java.awt.Image;
import java.awt.Rectangle;
import java.awt.event.MouseEvent;
import java.awt.image.BufferedImage;
import java.util.logging.Level;

import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SceneGraphPath;
import de.jreality.scene.Viewer;
import de.jreality.util.LoggingSystem;

/**
 * The software renderer component.
 * 
 * @version 1.0
 * @author <a href="mailto:hoffmann@math.tu-berlin.de">Tim Hoffmann</a>
 */
public class SoftViewer extends Component implements Runnable, Viewer {

    private static final long serialVersionUID = 1L;

    // TODO: remove ENFORCE_... ?

    private static final boolean ENFORCE_PAINT_ON_MOUSEEVENTS = false;

    // synchronizes the render thread
    private final Object renderAsyncLock = new Object();

    //  synchronizes the rendering
    private final Object renderImplLock = new Object();
    
    private SceneGraphPath cameraPath;

    private SceneGraphComponent root;

    private SceneGraphComponent auxiliaryRoot;

    private transient BufferedImage offscreen;

    private Renderer renderer;

    private boolean upToDate = false;

    private boolean backgroundExplicitlySet;

    private boolean imageValid;

    private Image bgImage;

    // be lazy in synchronous render: if there is a request but renderImpl is
    // still busy just return...
    private final boolean lazy;

    // TODO should lazy be the default?

    public SoftViewer() {
        this(false);
    }

    /**
     * create a soft viewer. possibly with lazy behaviour in synchronous
     * rendering: if there is a request but renderImpl is still busy just
     * return.
     * 
     * @param lazy
     *            if true lazy behaviour is enforced
     */
    public SoftViewer(boolean lazy) {
        super();
        this.lazy = lazy;
        setBackground(Color.white);
        if (ENFORCE_PAINT_ON_MOUSEEVENTS)
            enableEvents(AWTEvent.MOUSE_MOTION_EVENT_MASK);
        Thread renderThread = new Thread(this, "jReality render thread");
        // renderThread.setPriority(Thread.NORM_PRIORITY+1);
        renderThread.start();
    }

    public boolean isFocusable() {
        return true;
    }

    /*
     * (non-Javadoc)
     * 
     * @see de.jreality.soft.Viewer#getViewingComponent()
     */
    public Object getViewingComponent() {
        return this;
    }

    /*
     * (non-Javadoc)
     * 
     * @see de.jreality.soft.Viewer#setSceneRoot(de.jreality.scene.SceneGraphComponent)
     */
    public void setSceneRoot(SceneGraphComponent c) {
        root = c;
        if (renderer != null)
            renderer.setSceneRoot(c);
    }

    /*
     * (non-Javadoc)
     * 
     * @see de.jreality.soft.Viewer#getSceneRoot()
     */
    public SceneGraphComponent getSceneRoot() {
        return root;
    }

    public void render() {
        // System.out.println("render sync");

        if (EventQueue.isDispatchThread() || (lazy && isRendering)) {
            // avoid deadlock
            return;
        }
        renderImpl(getSize(),false);
        paintImmediately();
    }

    public void invalidate() {
        super.invalidate();
        imageValid = false;
        upToDate = false;
    }

    public boolean isDoubleBuffered() {
        return true;
    }

    public void paint(Graphics g) {
        if (!isShowing())
            return;
        Rectangle clip = g.getClipBounds();
        if (clip != null && clip.isEmpty())
            return;
        synchronized (this) {
            if (imageValid) {
                if (offscreen != null) {
                    if (bgImage != null)
                        g.drawImage(bgImage, 0, 0, Color.GREEN, null);
                    g.drawImage(offscreen, 0, 0, null);
                    return;
                } else
                    System.err.println("paint: no offscreen in paint");
            } else if (!upToDate)
                synchronized (renderAsyncLock) {
                    renderAsyncLock.notify();
                }
        }

    }

    public void update(Graphics g) {
        paint(g);
    }

    private boolean disposed;

    private int metric;

    public void run() {
        if (EventQueue.isDispatchThread()) {
            paintImmediately();
        } else
            while (true)
                try {
                    while (upToDate) {
                        try {
                            synchronized (renderAsyncLock) {
                                renderAsyncLock.wait();
                            }
                        } catch (InterruptedException e) {
                            e.printStackTrace();
                        }
                        if (disposed)
                            return;
                    }
                    upToDate = true;
                    renderImpl(getSize(),false);

                    // upToDate=true;
                    if (imageValid)
                        EventQueue.invokeLater(this);
                } catch (Exception ex) {
                    Thread t = Thread.currentThread();
                    t.getThreadGroup().uncaughtException(t, ex);
                }
    }

    private boolean isRendering = false;
    
    private void renderImpl(Dimension d,boolean quality) {
        synchronized (renderImplLock) {
        isRendering = true;
        //Dimension d = getSize();
        if (d.width > 0 && d.height > 0) {
            // System.out.println("render: off="+offscreen);
            if (offscreen == null || offscreen.getWidth() != d.width
                    || offscreen.getHeight() != d.height) {
                imageValid = false;

                offscreen = new BufferedImage(d.width, d.height,
                        BufferedImage.TYPE_INT_ARGB);
                offscreen.setAccelerationPriority(1.f);
                // TODO: findout if there is a way to keep the renderer...
                renderer = new Renderer(offscreen);
                renderer.setBestQuality(quality);

                Color c = getBackground();
                renderer.setBackgroundColor(c != null ? c.getRGB() : 0);
                renderer.setCameraPath(cameraPath);
                renderer.setSceneRoot(root);
                renderer.setAuxiliaryRoot(auxiliaryRoot);
            } else if (!backgroundExplicitlySet) {
                Color c = getBackground();// inherited from parent
                renderer.setBackgroundColor(c != null ? c.getRGB() : 0);
            }

            try {
                renderer.render();
            } catch (Exception e) {
                LoggingSystem.getLogger(this).log(Level.SEVERE,
                        "renderer.render() failed! ", e);

            }
            synchronized (this) {
                renderer.update();
                imageValid = true;
            }
        }
        isRendering = false;
        }
    }

    protected void processMouseMotionEvent(MouseEvent e) {
        super.processMouseMotionEvent(e);
        if (ENFORCE_PAINT_ON_MOUSEEVENTS)
            render();
    }

    public void addNotify() {
        super.addNotify();
        requestFocus();
    }

    protected void processMouseEvent(MouseEvent e) {
        super.processMouseEvent(e);
        switch (e.getID()) {
        case MouseEvent.MOUSE_CLICKED:
        case MouseEvent.MOUSE_PRESSED:
        case MouseEvent.MOUSE_RELEASED:
            requestFocus();
        }
        if (ENFORCE_PAINT_ON_MOUSEEVENTS)
            render();
    }

    public void setBackground(Color c) {
        super.setBackground(c);
        backgroundExplicitlySet = c != null;
        if (backgroundExplicitlySet && renderer != null)
            renderer.setBackgroundColor(c.getRGB());
    }

    private void paintImmediately() {
        if (!isShowing())
            return;

        Component c = this;
        Component parent;
        Rectangle bounds = new Rectangle(0, 0, getWidth(), getHeight());

        for (parent = c.getParent(); parent != null && c.isLightweight(); parent = c
                .getParent()) {
            bounds.x += c.getX();
            bounds.y += c.getY();
            c = parent;
        }
        Graphics gfx = c.getGraphics();
        gfx.setClip(bounds);
        c.paint(gfx);
    }

    /*
     * (non-Javadoc)
     * 
     * @see de.jreality.scene.Viewer#getCameraPath()
     */
    public SceneGraphPath getCameraPath() {
        return cameraPath;
    }

    /*
     * (non-Javadoc)
     * 
     * @see de.jreality.scene.Viewer#setCameraPath(de.jreality.util.SceneGraphPath)
     */
    public void setCameraPath(SceneGraphPath p) {
        cameraPath = p;
        if (renderer != null)
            renderer.setCameraPath(p);
    }

    /*
     * (non-Javadoc)
     * 
     * @see de.jreality.scene.Viewer#hasViewingComponent()
     */
    public boolean hasViewingComponent() {
        return true;
    }

    /*
     * (non-Javadoc)
     * 
     * @see de.jreality.scene.Viewer#initializeFrom(de.jreality.scene.Viewer)
     */
    public void initializeFrom(Viewer v) {
        setSceneRoot(v.getSceneRoot());
        setCameraPath(v.getCameraPath());
        setAuxiliaryRoot(v.getAuxiliaryRoot());
//        setMetric(v.getMetric());
    }

    /*
     * (non-Javadoc)
     * 
     * @see de.jreality.scene.Viewer#getMetric()
     */
    public int getMetric() {
        return metric;
    }

    /*
     * (non-Javadoc)
     * 
     * @see de.jreality.scene.Viewer#setMetric(int)
     */
    public void setMetric(int sig) {
        metric = sig;
    }

    /*
     * (non-Javadoc)
     * 
     * @see de.jreality.scene.Viewer#setAuxiliaryRoot(de.jreality.scene.SceneGraphComponent)
     */
    public void setAuxiliaryRoot(SceneGraphComponent ar) {
        auxiliaryRoot = ar;
        if (renderer != null)
            renderer.setAuxiliaryRoot(ar);
    }

    /*
     * (non-Javadoc)
     * 
     * @see de.jreality.scene.Viewer#getAuxiliaryRoot()
     */
    public SceneGraphComponent getAuxiliaryRoot() {
        return auxiliaryRoot;
    }

    public Image getBackgroundImage() {
        return bgImage;
    }

    public void setBackgroundImage(Image bgImage) {
        this.bgImage = bgImage;
    }

    public Dimension getViewingComponentSize() {
        return getSize();
    }

    public boolean canRenderAsync() {
        return true;
    }

    public void renderAsync() {
        // System.out.println("->render async<-");
        synchronized (renderAsyncLock) {
            if (upToDate) {
                upToDate = false;
                renderAsyncLock.notify();
            }
            // TODO: Bloch warns of using Thread.yield()... but this
            // makes the rendering smooth in oorange (with jgimmick timer)
            Thread.yield(); // encourage the render thread to do it's work
        }
    }

    public void dispose() {
        synchronized (renderAsyncLock) {
            disposed = true;
            renderAsyncLock.notify();
        }
    }
    
    public BufferedImage renderOffscreen(int width, int height) {
        
        renderImpl(new Dimension(width,height),true);
        BufferedImage bi = offscreen;
        render();
        return bi;
    }
}
