package de.jreality.jogl3;

import de.jreality.scene.Appearance;
import de.jreality.scene.event.AppearanceEvent;
import de.jreality.scene.event.AppearanceListener;
import de.jreality.scene.proxy.tree.SceneGraphNodeEntity;

public class JOGLAppearanceEntity extends SceneGraphNodeEntity implements AppearanceListener {

	public boolean dataUpToDate = false;
	
	protected JOGLAppearanceEntity(Appearance node) {
		super(node);
	}

	public void appearanceChanged(AppearanceEvent ev) {
		//System.out.println("Appearance changed");
		dataUpToDate = false;
		//mark all instances of this entity and all instances below as not upToData.
		
	}

}
