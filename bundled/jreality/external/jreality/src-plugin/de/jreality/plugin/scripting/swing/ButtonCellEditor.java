package de.jreality.plugin.scripting.swing;

import java.awt.Component;

import javax.swing.AbstractCellEditor;
import javax.swing.JLabel;
import javax.swing.JTable;
import javax.swing.table.TableCellEditor;

public class ButtonCellEditor extends AbstractCellEditor implements TableCellEditor {

	private static final long 	
		serialVersionUID = 1L;
	private JLabel
		defaultEditor = new JLabel("-");
	private Object 
		activeValue = null;
	
	@Override
	public Component getTableCellEditorComponent(JTable table,
			Object value, boolean isSelected, int row, int column) {
		this.activeValue = value;
		if (value instanceof Component) {
			return (Component)value;
		}
		return defaultEditor;
	}
	@Override
	public Object getCellEditorValue() {
		return activeValue;
	}
	
}