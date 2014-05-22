package de.jreality.swing;

import java.awt.AWTEvent;
import java.awt.AWTException;
import java.awt.BufferCapabilities;
import java.awt.BufferCapabilities.FlipContents;
import java.awt.Color;
import java.awt.Component;
import java.awt.Dimension;
import java.awt.EventQueue;
import java.awt.Font;
import java.awt.FontMetrics;
import java.awt.Frame;
import java.awt.Graphics;
import java.awt.GraphicsConfiguration;
import java.awt.Image;
import java.awt.Insets;
import java.awt.MenuBar;
import java.awt.Point;
import java.awt.Rectangle;
import java.awt.Toolkit;
import java.awt.event.PaintEvent;
import java.awt.image.BufferedImage;
import java.awt.image.ColorModel;
import java.awt.image.ImageObserver;
import java.awt.image.ImageProducer;
import java.awt.image.VolatileImage;
import java.awt.peer.ContainerPeer;

/**
 * 
 * There are many changes between java 5 and java 6, including many new classes not available in java 5.
 * So it is not possible to maintain one Fake Toolkit class for both versions.
 * Therefore, only one of the FakeToolkit5/6 and FakeFramePeer5/6 will compile depending on the used JDK.
 * 
 * @author Steffen Weissmann
 *
 */
class FakeFramePeer {
    private static final boolean DUMP = false;
	private BufferedImage bi;
    private VolatileImage vi;
    private JFakeFrame frame;
    private Runnable repaintAction;
    Rectangle bounds;
	private Image backBuffer;
    
	int state=Frame.NORMAL;
	
    FakeFramePeer(JFakeFrame f) {
        frame = f;
        Dimension d = f.getSize();
        //TODO this is ugly probably it is better to not crate the image
        // and check on existence in the getGraphics etc.
        if(d.width==0) d.width=1;
        if(d.height==0) d.height=1;
        bounds = new Rectangle(d.width, d.height);
        bi= new BufferedImage(d.width,d.height,BufferedImage.TYPE_INT_ARGB);
        backBuffer= new BufferedImage(d.width,d.height,BufferedImage.TYPE_INT_ARGB);
        vi = new FakeVolatileImage(bi);
    }

    public void endLayout() {
        if (DUMP) System.err.println("JFakeFramePeer end layout");
        frame.paint(bi.getGraphics());
        if(repaintAction != null)
            repaintAction.run();
    }

    public void endValidate() {
      if (DUMP) System.err.println("JFakeFramePeer end validate");
        frame.paint(bi.getGraphics());
        if(repaintAction != null)
            repaintAction.run();

    }

    public Insets getInsets() {
    	if (DUMP) System.out.println("FakeFramePeer.getInsets()");
        return new Insets(0,0,0,0);
    }

    public Insets insets() {
    	if (DUMP) System.out.println("FakeFramePeer.insets()");
        return new Insets(0,0,0,0);
    }

    public void setBounds(int x, int y, int width, int height) {
      if (DUMP) System.out.println("JFakeFrame set Bounds "+x+" "+y+" "+width+" "+height);
      boolean resized = (width!=bounds.width || height!=bounds.height);
      boolean moved = (x!=bounds.x || y!=bounds.y);
      if(resized) {
            bi =new BufferedImage(Math.max(1, width), Math.max(1, height),BufferedImage.TYPE_INT_ARGB);
            vi=new FakeVolatileImage(bi);
            backBuffer = new BufferedImage(Math.max(1, width), Math.max(1, height),BufferedImage.TYPE_INT_ARGB);
        }
      if (resized || moved) {
    	  bounds.setBounds(x, y, width, height);
      }
    }

    public Dimension getMinimumSize() {
    	if (DUMP) System.out.println("FakeFramePeer.getMinimumSize()");
        return new Dimension(0,0);
    }

    public Dimension getPreferredSize() {
    	if (DUMP) System.out.println("FakeFramePeer.getPreferredSize()");
        return new Dimension(256,256);
    }

    public Dimension minimumSize() {
    	if (DUMP) System.out.println("FakeFramePeer.minimumSize()");
        return getMinimumSize();
    }

    public Dimension preferredSize() {
    	if (DUMP) System.out.println("FakeFramePeer.preferredSize()");
        return getPreferredSize();
    }

    public Graphics getGraphics() {
    	if (DUMP) System.out.println("FakeFramePeer.getGraphics()");
       // System.err.println("JFakeFramePeer getGraphics");
        //TODO why does this work???
        // repaint BEFORE graphics is handed away!
        // Turns out: Does not Work on 1.5: so we "invokeLater"...
        // gives a little performance penalty but what can we do?
        if(repaintAction != null)
            //repaintAction.run();
            EventQueue.invokeLater(repaintAction);
        return bi.createGraphics();
    }

    public void paint(Graphics g) {
    	if (DUMP) System.out.println("FakeFramePeer.paint()");
        g.drawImage(bi,0,0,null);
    }

    public void print(Graphics g) {
    	if (DUMP) System.out.println("FakeFramePeer.print()");
        g.drawImage(bi,0,0,null);
    }

    public GraphicsConfiguration getGraphicsConfiguration() {
    	if (DUMP) System.out.println("FakeFramePeer.getGraphicsConfiguration()");
        return frame.getGraphicsConfiguration();
    }

    public Image createImage(int width, int height) {
    	if (DUMP) System.out.println("FakeFramePeer.createImage()");
        return new BufferedImage(width, height, BufferedImage.TYPE_INT_ARGB);
    }

    public Point getLocationOnScreen() {
        Point ret = new Point(bounds.x, bounds.y);
        if (DUMP) System.out.println("FakeFramePeer.getLocationOnScreen(): "+ret);
        return ret;
    }

    public Toolkit getToolkit() {
        return FakeToolKit.getDefaultToolkit();
    }

    public void coalescePaintEvent(PaintEvent e) {
    	if (DUMP) System.err.println("JFakeFramePeer coalescePaintEvent");
        frame.paint(bi.getGraphics());
        if(repaintAction != null)
            repaintAction.run();
    }

    public ColorModel getColorModel() {
    	if (DUMP) System.out.println("FakeFramePeer.getColorModel()");
        return ColorModel.getRGBdefault();
    }

    public VolatileImage createVolatileImage(int width, int height) {
    	if (DUMP) System.out.println("FakeFramePeer.createVolatileImage()");
        return vi;
    }

    public FontMetrics getFontMetrics(Font font) {
    	if (DUMP) System.out.println("FakeFramePeer.getFontMetrics()");
        return Toolkit.getDefaultToolkit().getFontMetrics(font);
    }

    public Image createImage(ImageProducer producer) {
    	if (DUMP) System.out.println("FakeFramePeer.createImage()");
        return Toolkit.getDefaultToolkit().createImage(producer);
    }

    public BufferedImage getRootImage() {
    	if (DUMP) System.out.println("FakeFramePeer.getRootImage()");
        return bi;
    }

    public void setRepaintAction(Runnable r) {
    	if (DUMP) System.out.println("FakeFramePeer.setRepaintAction()");
        repaintAction = r;
    }

    public boolean requestWindowFocus() {
    	if (DUMP) System.out.println("FakeFramePeer.requestWindowFocus()");
    	return true;
    }

    public void setBounds(int x, int y, int width, int height, int op) {
      if (DUMP) System.out.println("FakeFramePeer.setBounds() op="+op);
      setBounds(x, y, width, height);
    }
    
	public int getState() {
		return state;
	}

	public void setBoundsPrivate(int x, int y, int width, int height) {
		// TODO Auto-generated method stub
		
	}

	public void setIconImage(Image im) {
		// TODO Auto-generated method stub
		
	}

	public void setMaximizedBounds(Rectangle bounds) {
		// TODO Auto-generated method stub
		
	}

	public void setMenuBar(MenuBar mb) {
		// TODO Auto-generated method stub
		
	}

	public void setResizable(boolean resizeable) {
		// TODO Auto-generated method stub
		
	}

	public void setState(int state) {
		this.state=state;
	}

	public void setTitle(String title) {
		// TODO Auto-generated method stub
		
	}

	public void toBack() {
		// TODO Auto-generated method stub
		
	}

	public void toFront() {
		// TODO Auto-generated method stub
		
	}

	public void updateAlwaysOnTop() {
		// TODO Auto-generated method stub
		
	}

	public void updateFocusableWindowState() {
		// TODO Auto-generated method stub
		
	}

	public void beginLayout() {
		// TODO Auto-generated method stub
		
	}

	public void beginValidate() {
		// TODO Auto-generated method stub
		
	}

	public void cancelPendingPaint(int x, int y, int w, int h) {
		// TODO Auto-generated method stub
		
	}

	public boolean isPaintPending() {
		// TODO Auto-generated method stub
		return false;
	}

	public boolean isRestackSupported() {
		// TODO Auto-generated method stub
		return false;
	}

	public void restack() {
		// TODO Auto-generated method stub
		
	}

	public boolean canDetermineObscurity() {
		// TODO Auto-generated method stub
		return false;
	}

	public int checkImage(Image img, int w, int h, ImageObserver o) {
		// TODO Auto-generated method stub
		return 0;
	}

	public void createBuffers(int numBuffers, BufferCapabilities caps) throws AWTException {
		// TODO Auto-generated method stub
		
	}

	public void destroyBuffers() {
		// TODO Auto-generated method stub
		
	}

	public void disable() {
		// TODO Auto-generated method stub
		
	}

	public void dispose() {
		// TODO Auto-generated method stub
		
	}

	public void enable() {
		// TODO Auto-generated method stub
		
	}

	public void flip(FlipContents flipAction) {
		// TODO Auto-generated method stub
		
	}

	public Image getBackBuffer() {
		return backBuffer;
	}

	public Rectangle getBounds() {
		return new Rectangle(bounds);
	}

	public void handleEvent(AWTEvent e) {
		// TODO Auto-generated method stub
		
	}

	public boolean handlesWheelScrolling() {
		// TODO Auto-generated method stub
		return false;
	}

	public void hide() {
		frame.setMute(true);
	}

	public boolean isFocusable() {
		// TODO Auto-generated method stub
		return false;
	}

	public boolean isObscured() {
		// TODO Auto-generated method stub
		return false;
	}

	public boolean isReparentSupported() {
		// TODO Auto-generated method stub
		return false;
	}

	public void layout() {
		// TODO Auto-generated method stub
		
	}

	public boolean prepareImage(Image img, int w, int h, ImageObserver o) {
		// TODO Auto-generated method stub
		return false;
	}

	public void repaint(long tm, int x, int y, int width, int height) {
		// TODO Auto-generated method stub
		
	}

	public void reparent(ContainerPeer newContainer) {
		// TODO Auto-generated method stub
		
	}

	public boolean requestFocus(Component lightweightChild, boolean temporary, boolean focusedWindowChangeAllowed, long time) {
		// TODO Auto-generated method stub
		return false;
	}

	public void reshape(int x, int y, int width, int height) {
		// TODO Auto-generated method stub
		
	}

	public void setBackground(Color c) {
		// TODO Auto-generated method stub
		
	}

	public void setEnabled(boolean b) {
		// TODO Auto-generated method stub
		
	}

	public void setFont(Font f) {
		// TODO Auto-generated method stub
		
	}

	public void setForeground(Color c) {
		// TODO Auto-generated method stub
		
	}

	public void setVisible(boolean b) {
		// TODO Auto-generated method stub
		
	}

	public void show() {
		frame.setMute(false);
	}

	public void updateCursorImmediately() {
		// TODO Auto-generated method stub
		
	}

}
