package de.jreality.plugin.scripting.swing;

import java.awt.Component;

import javax.swing.JButton;
import javax.swing.JTable;
import javax.swing.table.DefaultTableCellRenderer;

public class ButtonCellRenderer extends DefaultTableCellRenderer {

	private static final long 	
		serialVersionUID = 1L;
	private JButton 
		renderButton = new JButton();
	
	@Override
	public Component getTableCellRendererComponent(JTable table,
			Object value, boolean isSelected, boolean hasFocus, int row,
			int column) {
		if (value instanceof JButton) {
			JButton buttonValue = (JButton)value;
			renderButton.setIcon(buttonValue.getIcon());
			renderButton.setText(buttonValue.getText());
			return renderButton;
		} else {
			return super.getTableCellRendererComponent(table, "", isSelected, hasFocus, row, column);
		}
	}
	
	@Override
	public void updateUI() {
		super.updateUI();
		if (renderButton != null) {
			renderButton.updateUI();
		}
	}
	
}