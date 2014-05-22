package de.jreality.plugin.basic;

import java.awt.Dimension;
import java.awt.GridLayout;
import java.beans.Expression;
import java.beans.Statement;
import java.lang.reflect.Method;
import java.lang.reflect.Modifier;
import java.util.Arrays;
import java.util.Comparator;

import javax.swing.BorderFactory;
import javax.swing.JComponent;

import de.jreality.plugin.icon.ImageHook;
import de.jreality.ui.viewerapp.BeanShell;
import de.jtem.jrworkspace.plugin.Controller;
import de.jtem.jrworkspace.plugin.PluginInfo;
import de.jtem.jrworkspace.plugin.sidecontainer.SideContainerPerspective;
import de.jtem.jrworkspace.plugin.sidecontainer.template.ShrinkPanelPlugin;

public class Shell extends ShrinkPanelPlugin {

	public Shell() {
		setInitialPosition(SHRINKER_BOTTOM);
		shrinkPanel.setShrinked(true);
	}
	
	@Override
	public Class<? extends SideContainerPerspective> getPerspectivePluginClass() {
		return View.class;
	}
	
	public static class H {
	
		public static String help(Object o) {
			class MethodComparator implements Comparator<Method> {
				public int compare(Method o1, Method o2) {
					return o1.getName().compareTo(o2.getName());
				}
			};
			Method[] methods = o.getClass().getDeclaredMethods();
			Arrays.sort(methods, new MethodComparator());
			StringBuffer r = new StringBuffer();
			for (Method m : methods) {
				if (!Modifier.isPublic(m.getModifiers())) {
					continue;
				}
				r.append(m.getName() + "(");
				int num = 0;
				for (Class<?> param : m.getParameterTypes()) {
					r.append(param.getSimpleName());
					if (++num != m.getParameterTypes().length) {
						 r.append(", ");
					}
				}
				r.append(")\n");
			}
			return r.toString();
		}
		
	}
		
	@Override
	public void install(Controller c) throws Exception {
		super.install(c);
		View sceneView = c.getPlugin(View.class);
		BeanShell beanShell = new BeanShell(sceneView.getSelectionManager());

		beanShell.eval("import de.jreality.geometry.*;");
		beanShell.eval("import de.jreality.math.*;");    
		beanShell.eval("import de.jreality.scene.*;");
		beanShell.eval("import de.jreality.scene.data.*;");
		beanShell.eval("import de.jreality.scene.tool.*;");
		beanShell.eval("import de.jreality.shader.*;");
		beanShell.eval("import de.jreality.tools.*;");
		beanShell.eval("import de.jreality.util.*;");
		beanShell.eval("import de.jreality.plugin.basic.Shell.H;");
		beanShell.eval("import de.jreality.plugin.*");
		beanShell.eval("import de.jreality.plugin.audio.*");
		beanShell.eval("import de.jreality.plugin.view.*");
		beanShell.eval("import de.jreality.plugin.vr.*");

		//set some objects to be accessible from within the beanShell
		try {
			Object bshEval = new Expression(beanShell, "getBshEval", null).getValue();
			Object interpreter = new Expression(bshEval, "getInterpreter", null).getValue();
			new Statement(interpreter, "set", new Object[]{"_controller", c}).execute();
			new Statement(interpreter, "set", new Object[]{"_viewer", sceneView.getViewer()}).execute();
		}
		catch (Exception e) { e.printStackTrace(); }

		JComponent shell = (JComponent) beanShell.getComponent();
		shell.setPreferredSize(new Dimension(0, 100));
		shell.setMinimumSize(new Dimension(10, 100));

		shrinkPanel.getContentPanel().setBorder(BorderFactory.createEtchedBorder());
		shrinkPanel.setLayout(new GridLayout());
		shrinkPanel.add(shell);
	}


	@Override
	public void uninstall(Controller c) throws Exception {
		super.uninstall(c);
		shrinkPanel.removeAll();
	}

	@Override
	public PluginInfo getPluginInfo() {
		PluginInfo info = new PluginInfo();
		info.name = "BeanShell";
		info.vendorName = "Stefan Sechelmann";
		info.icon = ImageHook.getIcon("application_osx_terminal.png");
		return info;
	}
	
	
	@Override
	public String getHelpDocument() {
		return "BeanShell.html";
	}
	
	@Override
	public String getHelpPath() {
		return "/de/jreality/plugin/help/";
	}
	
	@Override
	public Class<?> getHelpHandle() {
		return getClass();
	}
	

}

