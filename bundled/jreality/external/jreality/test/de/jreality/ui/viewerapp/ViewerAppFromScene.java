/*
 * Created on 01.11.2006
 *
 * This file is part of the de.jreality.ui.viewerapp package.
 * 
 * This program is free software; you can redistribute and/or modify 
 * it under the terms of the GNU General Public License as published 
 * by the Free Software Foundation; either version 2 of the license, or
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITTNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License 
 * along with this program; if not, write to the 
 * Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307
 * USA 
 */


package de.jreality.ui.viewerapp;

import de.jreality.geometry.Primitives;
import de.jreality.io.JrScene;
import de.jreality.io.JrSceneFactory;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.SceneGraphComponent;


public class ViewerAppFromScene {

	public static void main(String[] args) {
        IndexedFaceSet cube = Primitives.coloredCube();
        SceneGraphComponent sgc = new SceneGraphComponent();
        sgc.setGeometry(cube);
        ViewerApp app1 = new ViewerApp(sgc);
        app1.update();
        app1.display();
        
        
        JrScene scene = JrSceneFactory.getDefaultDesktopScene();
        scene.getSceneRoot().getChildComponent(0).addChild(sgc);  //tools are attached to scene node
        ViewerApp app2 = new ViewerApp(scene);
        app2.update();
        app2.display();
    }
}