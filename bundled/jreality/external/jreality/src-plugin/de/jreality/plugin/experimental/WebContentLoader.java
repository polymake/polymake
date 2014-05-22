package de.jreality.plugin.experimental;

import java.awt.Dimension;
import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.awt.Insets;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.io.IOException;
import java.io.InputStream;
import java.net.URI;
import java.net.URISyntaxException;
import java.util.LinkedList;
import java.util.List;

import javax.swing.AbstractListModel;
import javax.swing.JButton;
import javax.swing.JCheckBox;
import javax.swing.JList;
import javax.swing.JScrollPane;
import javax.swing.JTextField;
import javax.swing.text.AttributeSet;
import javax.swing.text.html.HTML.Attribute;
import javax.swing.text.html.HTML.Tag;
import javax.swing.text.html.HTMLDocument;
import javax.swing.text.html.HTMLEditorKit;

import de.jreality.plugin.JRViewer;
import de.jreality.plugin.JRViewer.ContentType;
import de.jreality.plugin.JRViewerUtility;
import de.jreality.plugin.basic.Content;
import de.jreality.plugin.basic.Scene;
import de.jreality.plugin.basic.View;
import de.jreality.reader.Readers;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.util.Input;
import de.jtem.jrworkspace.plugin.Controller;
import de.jtem.jrworkspace.plugin.PluginInfo;
import de.jtem.jrworkspace.plugin.annotation.Experimental;
import de.jtem.jrworkspace.plugin.sidecontainer.SideContainerPerspective;
import de.jtem.jrworkspace.plugin.sidecontainer.template.ShrinkPanelPlugin;


@Experimental
public class WebContentLoader extends ShrinkPanelPlugin implements ActionListener {

	private Content 
		contentPlugin = null;
	private Scene
		scene = null;
	private JTextField
		locationField = new JTextField("http://www.sechel.de/models/");
	private JList
		contentList = new JList(new ContentModel());
	private JScrollPane
		contentScroller = new JScrollPane(contentList);
	private HTMLEditorKit
		htmlEditorKit = new HTMLEditorKit();
	private List<WebModel>
		content = new LinkedList<WebModel>();
	private JCheckBox
		encompassChecker = new JCheckBox("Encompass After Load", true);
	private JButton
		loadButton = new JButton("Load Model");
	
	
	public WebContentLoader() {
		shrinkPanel.setLayout(new GridBagLayout());
		GridBagConstraints c = new GridBagConstraints();
		c.fill = GridBagConstraints.HORIZONTAL;
		c.weightx = 1.0;
		c.gridwidth = GridBagConstraints.REMAINDER;
		c.insets = new Insets(2,2,2,2);
		
		
		contentScroller.setPreferredSize(new Dimension(10, 150));
		shrinkPanel.add(locationField, c);
		c.weighty = 1.0;
		c.fill = GridBagConstraints.BOTH;
		shrinkPanel.add(contentScroller, c);
		c.weighty = 0.0;
		c.fill = GridBagConstraints.HORIZONTAL;
		shrinkPanel.add(encompassChecker, c);
		shrinkPanel.add(loadButton, c);
		
		locationField.addActionListener(this);
		loadButton.addActionListener(this);
	}
	
	public void actionPerformed(ActionEvent e) {
		if (locationField == e.getSource()) {
			updateContent();
		}
		if (loadButton == e.getSource()) {
			Object[] models = contentList.getSelectedValues();
			SceneGraphComponent root = new SceneGraphComponent("Web Models");
			for (Object model : models) {
				WebModel wm = (WebModel)model;
				try {
					Input in = Input.getInput(wm.uri.toURL());
					SceneGraphComponent c = Readers.read(in);
					root.addChild(c);
				} catch (Exception ex) {
					System.out.println("Cannot load model " + wm.name + ": " + ex.getMessage());
				}
			}
			contentPlugin.setContent(root);
			if (encompassChecker.isSelected()) {
				JRViewerUtility.encompassEuclidean(scene);
			}
			
		}
	}
	
	private void updateContent() {
		content.clear();
		URI siteUrl = null;
		try {
			siteUrl = new URI(locationField.getText());
		} catch (URISyntaxException e) {
			content.add(new WebModel(null, "Invalid URI"));
			contentList.revalidate();
			return;
		}
		InputStream in = null; 
		try {
			in = siteUrl.toURL().openStream();
		} catch (IOException e) {
			content.add(new WebModel(null, "Cannot open URL"));
			contentList.revalidate();
			return;
		}
		HTMLDocument doc =(HTMLDocument) htmlEditorKit.createDefaultDocument();
		try {
			htmlEditorKit.read(in, doc, 0);
		} catch (Exception e) {
			content.add(new WebModel(null, "Cannot read web page"));
			contentList.revalidate();
			return;
		}
		HTMLDocument.Iterator linkIt = doc.getIterator(Tag.A);
		for (; linkIt.isValid(); linkIt.next()) {
			AttributeSet aSet = linkIt.getAttributes();
			String href = (String)aSet.getAttribute(Attribute.HREF);
			try {
				URI uriRef = new URI(href);
				if (Readers.findFormat(uriRef.toString()) == null) {
					continue;
				}
				uriRef = siteUrl.resolve(uriRef);
				String name = uriRef.toURL().getFile();
				WebModel model = new WebModel(uriRef, name);
				content.add(model);
			} catch (Exception e) {
				e.printStackTrace();
			}
		}
		contentList.revalidate();
	}
	
	
	private class WebModel {
		
		public URI
			uri = null;
		public String
			name = "NoName";

		public WebModel(URI url, String name) {
			this.uri = url;
			this.name = name;
		}
		
		@Override
		public String toString() {
			return name;
		}
		
	}
	
	private class ContentModel extends AbstractListModel {

		private static final long 
			serialVersionUID = 1L;

		public Object getElementAt(int index) {
			return content.get(index);
		}

		public int getSize() {
			return content.size();
		}
		
	}
	
	
	@Override
	public void install(Controller c) throws Exception {
		super.install(c);
		contentPlugin = JRViewerUtility.getContentPlugin(c);
		scene = c.getPlugin(Scene.class);
		updateContent();
	}
	
	
	@Override
	public void storeStates(Controller c) throws Exception {
		super.storeStates(c);
		c.storeProperty(getClass(), "location", locationField.getText());
	}

	
	@Override
	public void restoreStates(Controller c) throws Exception {
		super.restoreStates(c);
		locationField.setText(c.getProperty(getClass(), "location", "http://www.sechel.de/models/"));
	}
	
	
	@Override
	public Class<? extends SideContainerPerspective> getPerspectivePluginClass() {
		return View.class;
	}

	@Override
	public PluginInfo getPluginInfo() {
		return new PluginInfo("Web Content Loader", "Stefan Sechelmann");
	}

	
	public static void main(String[] args) {
		JRViewer v = new JRViewer();
		v.addBasicUI();
		v.addContentSupport(ContentType.CenteredAndScaled);
		v.registerPlugin(new WebContentLoader());
		v.startup();
	}
	
}
