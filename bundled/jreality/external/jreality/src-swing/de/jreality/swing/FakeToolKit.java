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

import java.awt.Button;
import java.awt.Canvas;
import java.awt.Checkbox;
import java.awt.CheckboxMenuItem;
import java.awt.Choice;
import java.awt.Dialog;
import java.awt.Dimension;
import java.awt.EventQueue;
import java.awt.FileDialog;
import java.awt.Font;
import java.awt.FontMetrics;
import java.awt.Frame;
import java.awt.HeadlessException;
import java.awt.Image;
import java.awt.Label;
import java.awt.List;
import java.awt.Menu;
import java.awt.MenuBar;
import java.awt.MenuItem;
import java.awt.Panel;
import java.awt.PopupMenu;
import java.awt.PrintJob;
import java.awt.ScrollPane;
import java.awt.Scrollbar;
import java.awt.TextArea;
import java.awt.TextField;
import java.awt.Toolkit;
import java.awt.Window;
import java.awt.datatransfer.Clipboard;
import java.awt.dnd.DragGestureEvent;
import java.awt.dnd.InvalidDnDOperationException;
import java.awt.dnd.peer.DragSourceContextPeer;
import java.awt.im.InputMethodHighlight;
import java.awt.image.ColorModel;
import java.awt.image.ImageObserver;
import java.awt.image.ImageProducer;
import java.awt.peer.ButtonPeer;
import java.awt.peer.CanvasPeer;
import java.awt.peer.CheckboxMenuItemPeer;
import java.awt.peer.CheckboxPeer;
import java.awt.peer.ChoicePeer;
import java.awt.peer.DialogPeer;
import java.awt.peer.FileDialogPeer;
import java.awt.peer.FontPeer;
import java.awt.peer.LabelPeer;
import java.awt.peer.ListPeer;
import java.awt.peer.MenuBarPeer;
import java.awt.peer.MenuItemPeer;
import java.awt.peer.MenuPeer;
import java.awt.peer.PanelPeer;
import java.awt.peer.PopupMenuPeer;
import java.awt.peer.ScrollPanePeer;
import java.awt.peer.ScrollbarPeer;
import java.awt.peer.TextAreaPeer;
import java.awt.peer.TextFieldPeer;
import java.awt.peer.WindowPeer;
import java.net.URL;
import java.util.Map;
import java.util.Properties;
import java.util.logging.Level;

import de.jreality.util.LoggingSystem;

abstract class FakeToolKit extends Toolkit {
	
    protected static  FakeToolKit ftk;
    
    static {
    	try {
			ftk = (FakeToolKit) Class.forName("de.jreality.swing.FakeToolKit6").newInstance();
    	} catch (Throwable t) {
    		try {
				ftk = (FakeToolKit) Class.forName("de.jreality.swing.FakeToolKit5").newInstance();
			} catch (Exception e) {
				throw  new ExceptionInInitializerError(e);
			}
    	}
    	LoggingSystem.getLogger(FakeToolKit.class).log(Level.CONFIG, "created Toolkit: "+ftk);
	}
    
    public static Toolkit getDefaultToolkit() {
        return ftk;
    }
    protected Toolkit tk = Toolkit.getDefaultToolkit();
    public FakeToolKit() {
        super();
    }

    public int getScreenResolution() throws HeadlessException {
        return tk.getScreenResolution();
    }

    public void beep() {
        tk.beep();

    }

    public void sync() {
        tk.sync();
    }

    public Dimension getScreenSize() throws HeadlessException {
        return tk.getScreenSize();
    }

    protected EventQueue getSystemEventQueueImpl() {
        return tk.getSystemEventQueue();
    }

    public Image createImage(byte[] imagedata, int imageoffset, int imagelength) {
        return tk.createImage(imagedata,imageoffset,imagelength);
    }

    public Clipboard getSystemClipboard() throws HeadlessException {
        return tk.getSystemClipboard();
    }

    public ColorModel getColorModel() throws HeadlessException {
        return tk.getColorModel();
    }

    public String[] getFontList() {
        return tk.getFontList();
    }

    public FontMetrics getFontMetrics(Font font) {
        return tk.getFontMetrics(font);
    }

    public Image createImage(ImageProducer producer) {
        return tk.createImage(producer);
    }

    public Image createImage(String filename) {
        return tk.createImage(filename);
    }

    public Image getImage(String filename) {
        return tk.getImage(filename);
    }

    public Image createImage(URL url) {
        return tk.createImage(url);
    }

    public Image getImage(URL url) {
        return tk.getImage(url);
    }

    public DragSourceContextPeer createDragSourceContextPeer(
            DragGestureEvent dge) throws InvalidDnDOperationException {
        return tk.createDragSourceContextPeer(dge);
    }

    public int checkImage(Image image, int width, int height,
            ImageObserver observer) {
        return tk.checkImage(image,width,height,observer);
    }

    public boolean prepareImage(Image image, int width, int height,
            ImageObserver observer) {
        return tk.prepareImage(image,width,height,observer);
    }

    protected ButtonPeer createButton(Button target) throws HeadlessException {
        return null;
    }

    protected CanvasPeer createCanvas(Canvas target) {
        return null;
    }

    protected CheckboxMenuItemPeer createCheckboxMenuItem(
            CheckboxMenuItem target) throws HeadlessException {
        return null;
    }

    protected CheckboxPeer createCheckbox(Checkbox target)
            throws HeadlessException {
        return null;
    }

    protected ChoicePeer createChoice(Choice target) throws HeadlessException {
        return null;
    }

    protected DialogPeer createDialog(Dialog target) throws HeadlessException {
        return null;
    }

    protected FileDialogPeer createFileDialog(FileDialog target)
            throws HeadlessException {
        return null;
    }

    protected FontPeer getFontPeer(String name, int style) {
        return null;
    }

    protected LabelPeer createLabel(Label target) throws HeadlessException {
        // TODO Auto-generated method stub
        return null;
    }

    protected ListPeer createList(List target) throws HeadlessException {
        // TODO Auto-generated method stub
        return null;
    }

    protected MenuBarPeer createMenuBar(MenuBar target)
            throws HeadlessException {
        // TODO Auto-generated method stub
        return null;
    }

    protected MenuItemPeer createMenuItem(MenuItem target)
            throws HeadlessException {
        // TODO Auto-generated method stub
        return null;
    }

    protected MenuPeer createMenu(Menu target) throws HeadlessException {
        // TODO Auto-generated method stub
        return null;
    }

    protected PanelPeer createPanel(Panel target) {
        // TODO Auto-generated method stub
        return null;
    }

    protected PopupMenuPeer createPopupMenu(PopupMenu target)
            throws HeadlessException {
        // TODO Auto-generated method stub
        return null;
    }

    protected ScrollPanePeer createScrollPane(ScrollPane target)
            throws HeadlessException {
        // TODO Auto-generated method stub
        return null;
    }

    protected ScrollbarPeer createScrollbar(Scrollbar target)
            throws HeadlessException {
        // TODO Auto-generated method stub
        return null;
    }

    protected TextAreaPeer createTextArea(TextArea target)
            throws HeadlessException {
        // TODO Auto-generated method stub
        return null;
    }

    protected TextFieldPeer createTextField(TextField target)
            throws HeadlessException {
        // TODO Auto-generated method stub
        return null;
    }

    protected WindowPeer createWindow(Window target) throws HeadlessException {
        // TODO Auto-generated method stub
        return null;
    }

    public Map mapInputMethodHighlight(InputMethodHighlight highlight)
            throws HeadlessException {
        // TODO Auto-generated method stub
        return null;
    }

    public PrintJob getPrintJob(Frame frame, String jobtitle, Properties props) {
        // TODO Auto-generated method stub
        return null;
    }
    
    /*
	@Override
	protected DesktopPeer createDesktopPeer(Desktop target) throws HeadlessException {
		return null;
	}

	@Override
	public boolean isModalExclusionTypeSupported(ModalExclusionType modalExclusionType) {
		return false;
	}

	@Override
	public boolean isModalityTypeSupported(ModalityType modalityType) {
		return false;
	};
	*/
    
}
