package de.jreality.plugin.basic;

import java.awt.event.ActionEvent;

import de.jreality.plugin.icon.ImageHook;
import de.jreality.ui.viewerapp.actions.AbstractJrToggleAction;
import de.jtem.jrworkspace.plugin.Controller;
import de.jtem.jrworkspace.plugin.PluginInfo;
import de.jtem.jrworkspace.plugin.aggregators.ToolBarAggregator;
import de.jtem.jrworkspace.plugin.flavor.PerspectiveFlavor;

public class ViewToolBar extends ToolBarAggregator {

	@Override
	public PluginInfo getPluginInfo() {
		return new PluginInfo("View Tool Bar", "Stefan Sechelmann");
	}

	@Override
	public Class<? extends PerspectiveFlavor> getPerspective() {
		return View.class;
	}

	@Override
	public void install(Controller c) throws Exception {
		super.install(c);
		final View view = c.getPlugin(View.class);
		addTool(ViewToolBar.class, 1000.0, new AbstractJrToggleAction("Left Slot") {
			private static final long serialVersionUID = 1L;
			{
				setIcon(ImageHook.getIcon("application_side_boxes_left.png"));
				setSelected(view.isShowLeft());
			}
			@Override
			public void actionPerformed(ActionEvent e) {
				view.setShowLeft(isSelected());
			}
		}.createToolboxItem());
		addTool(ViewToolBar.class, 1001.0, new AbstractJrToggleAction("Right Slot") {
			private static final long serialVersionUID = 1L;
			{
				setIcon(ImageHook.getIcon("application_side_boxes_right.png"));
				setSelected(view.isShowRight());
			}
			@Override
			public void actionPerformed(ActionEvent e) {
				view.setShowRight(isSelected());
			}
		}.createToolboxItem());
		addTool(ViewToolBar.class, 1002.0, new AbstractJrToggleAction("Top Slot") {
			private static final long serialVersionUID = 1L;
			{
				setIcon(ImageHook.getIcon("application_side_boxes_top.png"));
				setSelected(view.isShowTop());
			}
			@Override
			public void actionPerformed(ActionEvent e) {
				view.setShowTop(isSelected());
			}
		}.createToolboxItem());
		addTool(ViewToolBar.class, 1003.0, new AbstractJrToggleAction("Bottom Slot") {
			private static final long serialVersionUID = 1L;
			{
				setIcon(ImageHook.getIcon("application_side_boxes_bottom.png"));
				setSelected(view.isShowBottom());
			}
			@Override
			public void actionPerformed(ActionEvent e) {
				view.setShowBottom(isSelected());
			}
		}.createToolboxItem());
	}
	
	@Override
	public double getToolBarPriority() {
		return -10.0;
	}
}
