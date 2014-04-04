package de.jreality.ui.viewerapp.actions;

import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;

import javax.swing.AbstractButton;
import javax.swing.Action;
import javax.swing.JCheckBoxMenuItem;
import javax.swing.JMenuItem;
import javax.swing.JToggleButton;
import javax.swing.JToggleButton.ToggleButtonModel;
import javax.swing.KeyStroke;

// this is quite a hack, in java 6 the button model
// would not be required. Then, the action has a
// selected state (Action.SELECTED_KEY)
public abstract class AbstractJrToggleAction extends AbstractJrAction {

	private static final long 
		serialVersionUID = 1L;
	ToggleButtonModel model=new ToggleButtonModel();
	
	public AbstractJrToggleAction(String name) {
		super(name);
		setSelected(false);
		model.addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent e) {
				AbstractJrToggleAction.this.actionPerformed(e);
			}
		});
	}

	public void setSelected(boolean value) {
		model.setSelected(value);
	}
	  
	public boolean isSelected() {
	  return model.isSelected();
	}

	@Override
	public JMenuItem createMenuItem() {
		JCheckBoxMenuItem ret = new JCheckBoxMenuItem();
		ret.setText((String) getValue(Action.NAME));
		ret.setIcon(getIcon());
		ret.setModel(model);
		ret.setAccelerator((KeyStroke)getValue(ACCELERATOR_KEY));
		return ret;
	}
	
	@Override
	public AbstractButton createToolboxItem() {
		JToggleButton ret = new JToggleButton();
		if (getIcon() != null) {
			ret.setIcon(getIcon());
			ret.setToolTipText((String) getValue(Action.NAME));
		} else {
			ret.setText((String) getValue(Action.NAME));
		}
		ret.setModel(model);
		return ret;
	}
}
