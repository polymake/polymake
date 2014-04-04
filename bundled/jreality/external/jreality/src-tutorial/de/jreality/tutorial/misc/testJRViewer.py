# This example demonstrates the use of jReality with jython.
# First download and install jython (www.jython.org).
# Then, take and look at the file 'myjython' in this directory
# By editing the paths there to reflect your configuration,
# you should be able to execute:
#  myjython testViewerApp.py
# A jReality ViewerApp window should appear with a red icosahedron in it.
if __name__=='__main__':
    import de.jreality.scene
    import de.jreality.plugin
    import java.awt
    sgc = de.jreality.scene.SceneGraphComponent("test")
    sgc.setGeometry(de.jreality.geometry.Primitives.icosahedron())
    ap = de.jreality.scene.Appearance()
    ap.setAttribute("polygonShader.diffuseColor", java.awt.Color.RED)
    sgc.setAppearance(ap)
    de.jreality.plugin.JRViewer.display(sgc)


