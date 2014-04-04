package de.jreality.ui.viewerapp;

import de.jreality.scene.SceneGraphPath;

public interface SelectionManager  {

	public abstract Selection getDefaultSelection();

	public abstract void setDefaultSelection(Selection defaultSelection);

	public abstract Selection getSelection();

	public abstract void setSelection(Selection selection);
	
	public abstract SceneGraphPath getDefaultSelectionPath();

	public abstract void setDefaultSelectionPath(SceneGraphPath defaultSelection);

	public abstract SceneGraphPath getSelectionPath();

	public abstract void setSelectionPath(SceneGraphPath selection);

	public abstract void addSelectionListener(SelectionListener listener);

	public abstract void removeSelectionListener(SelectionListener listener);

	public abstract boolean isRenderSelection();

	public abstract void setRenderSelection(boolean renderSelection);

	// cycling functionality
	public abstract void addSelection(SceneGraphPath p);

	public abstract void removeSelection(SceneGraphPath p);

	public abstract void clearSelections();

	public abstract void cycleSelection();

	public abstract void cycleSelectionPath();
}