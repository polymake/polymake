/**
 *
 * This file is part of jReality. jReality is open source software, made
 * available under a BSD license:
 *
 * Copyright (c) 2003-2006, jReality Group: Charles Gunn, Tim Hoffmann, Markus
 * Schmies, Steffen Weissmann.
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * - Neither the name of jReality nor the names of its contributors nor the
 *   names of their associated organizations may be used to endorse or promote
 *   products derived from this software without specific prior written
 *   permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */


package de.jreality.scene.proxy.tree;

import java.lang.management.ManagementFactory;
import java.lang.management.MemoryMXBean;
import java.lang.ref.ReferenceQueue;
import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;

import junit.framework.TestCase;
import de.jreality.Debug;
import de.jreality.examples.CatenoidHelicoid;
import de.jreality.plugin.JRViewer;
import de.jreality.plugin.content.ContentAppearance;
import de.jreality.plugin.content.ContentLoader;
import de.jreality.plugin.content.ContentTools;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.Sphere;
import de.jreality.scene.proxy.ProxyFactory;
import de.jreality.tools.RotateTool;
import de.jreality.toolsystem.ToolUpdateProxy;

/**
 *
 * TODO: comment this
 *
 * @author weissman
 *
 */
public class TreeProxyTest extends TestCase {

  MemoryMXBean mbean = ManagementFactory.getMemoryMXBean();

  public static void main(String[] args) {
    junit.textui.TestRunner.run(TreeProxyTest.class);
  }
  
  static class PrintFactory extends ProxyFactory {
    public Object getProxy() {
      //System.out.println("PrintFactory.getProxy()");
      return null;
    }
  }
  
  static class TreeDumper {
    StringBuffer indent=new StringBuffer(" ");
    void dumpTree(SceneTreeNode node) {
      boolean isValid=node.toPath().isValid();
      System.out.println(indent.substring(0, indent.length()-1)+"-"+node.getNode().getName()+"["+node.getNode().getClass().getName()+"] valid="+isValid);
      indent.append(" | ");
      if (!node.isLeaf())
        for (Iterator i = ((SceneTreeNode)node).getChildren().iterator(); i.hasNext(); )
          dumpTree((SceneTreeNode) i.next());
      indent.delete(indent.length()-3, indent.length());
    }
  }

  public void testTreeProxy() {
    if (true) return;
    SceneGraphComponent root = new SceneGraphComponent();
    SceneProxyTreeBuilder ttp = new SceneProxyTreeBuilder(root);
    ttp.setProxyTreeFactory(new ProxyTreeFactory());
    ttp.getProxyTreeFactory().setProxyFactory(new PrintFactory());
    ttp.setProxyConnector(new ProxyConnector());
    SceneTreeNode tn = ttp.createProxyTree();
    new TreeDumper().dumpTree(tn);
    System.out.println("++++++++++++++++++++++");
  }

  public void testTreeProxyMemLeak() {
    if (true) return;
    SceneGraphComponent root = new SceneGraphComponent();
    SceneGraphComponent c1 = new SceneGraphComponent();
    SceneGraphComponent c2 = new SceneGraphComponent();
    SceneGraphComponent c3 = new SceneGraphComponent();
    root.addChild(c1);
    root.addChild(c2);
    c1.addChild(c3);
    c2.addChild(c3);
    c1.addTool(new RotateTool());
    SceneProxyTreeBuilder ttp = new SceneProxyTreeBuilder(root);
    ttp.setProxyTreeFactory(new ProxyTreeFactory());
    ttp.getProxyTreeFactory().setProxyFactory(new PrintFactory());
    ttp.setProxyConnector(new ProxyConnector());
    @SuppressWarnings("unused") SceneTreeNode tn = ttp.createProxyTree();
    for (int i = 0; i < 1000; i++) {
        CatenoidHelicoid ch = new CatenoidHelicoid(10);
        c3.setGeometry(ch);
        c3.setGeometry(new CatenoidHelicoid(10));
        for (int j = 0; j < 15; j++) System.gc();
        if ((i%100) == 0) { System.out.println(mbean.getHeapMemoryUsage());
          Debug.ref(ttp, ch);
          System.out.println("----");
        }
      }
  }

  public void testTreeProxyMemLeakTreeView() {
    //if (true) return;
    SceneGraphComponent root = new SceneGraphComponent();
    SceneGraphComponent c1 = new SceneGraphComponent();
    SceneGraphComponent c2 = new SceneGraphComponent();
    SceneGraphComponent c3 = new SceneGraphComponent();
    root.setName("root");
    c1.setName("c1");
    c2.setName("c2");
    c3.setName("c3");
    root.addChild(c1);
    root.addChild(c2);
    c1.addChild(c3);
    c2.addChild(c3);
    c1.addTool(new RotateTool());
//    SceneTreeModel sm = new SceneTreeModel(root);
    for (int i = 0; i < 500; i++) {
      CatenoidHelicoid ch = new CatenoidHelicoid(10);
      ch.setName("ch["+i+"]");
      c3.setGeometry(ch);
      //for (int j = 0; j < 15; j++)
    	  System.gc();
      if ((i%100) == 0) { System.out.println(mbean.getHeapMemoryUsage());
//        Debug.ref(sm, ch);
      }
    }
  }

  static ReferenceQueue<Object> QUEUE = new ReferenceQueue<Object>();
  static List<X> REFS=new ArrayList<X>();
  static class X extends WeakReference<Object> {
    String s;
    X(Object o) { super(o, QUEUE); s=o.toString(); }
    @Override
    public String toString() {
      return s;
    }
  }
  public void testTreeProxyMemLeakToolProxy() {
    SceneGraphComponent root = new SceneGraphComponent();
    SceneGraphComponent c1 = new SceneGraphComponent();
    SceneGraphComponent c2 = new SceneGraphComponent();
    SceneGraphComponent c3 = new SceneGraphComponent();
    root.addChild(c1);
    root.addChild(c2);
    c1.addChild(c3);
    c2.addChild(c3);
    c1.addTool(new RotateTool());
    if (false) {
      ToolUpdateProxy sm = new ToolUpdateProxy(null);
      sm.setSceneRoot(root);
    } else {
      //System.setProperty(SystemProperties.VIEWER, SystemProperties.VIEWER_DEFAULT_SOFT);
	    JRViewer v = new JRViewer();
		v.addBasicUI();
		v.setContent(root);
		v.registerPlugin(new ContentAppearance());
		v.registerPlugin(new ContentLoader());
		v.registerPlugin(new ContentTools());
		v.startup();
	}
    for (int i = 0; i < 500; i++) {
      CatenoidHelicoid ch = new CatenoidHelicoid(10);
      REFS.add(new X(ch));
      c3.setGeometry(ch);
      System.gc();
      for(Object o=QUEUE.poll(); o!=null; o=QUEUE.poll()) {
//        System.out.println("collected: "+o);
        REFS.remove(o);
      }
      if ((i%100) == 0) 
        System.out.println(mbean.getHeapMemoryUsage());
    }
  }

  public void testTreeUpToDateProxy() {
    if (true) return;
    SceneGraphComponent root = new SceneGraphComponent();
    root.setName("root");
    SceneGraphComponent p1 = new SceneGraphComponent();
    p1.setName("p1");
    SceneGraphComponent p2 = new SceneGraphComponent();
    p2.setName("p2");
    SceneGraphComponent p3 = new SceneGraphComponent();
    p3.setName("p3");
    
    root.addChild(p1);
    root.addChild(p2);
    p2.setGeometry(new Sphere());
    
    UpToDateSceneProxyBuilder ttp = new UpToDateSceneProxyBuilder(root);
    
    TreeDumper td = new TreeDumper(); 
    ttp.setProxyTreeFactory(new ProxyTreeFactory());
    ttp.getProxyTreeFactory().setProxyFactory(new PrintFactory());
    ttp.setProxyConnector(new ProxyConnector());

    SceneTreeNode tn = ttp.createProxyTree();

    td.dumpTree(tn);
    System.out.println("created ++++++++++++++++++++++\n");

    root.addChild(p3);

    td.dumpTree(tn);
    System.out.println("added p3 to root ++++++++++++++++++++++\n");
    
    p1.addChild(p2);

    td.dumpTree(tn);
    System.out.println("added p2 to p1 ++++++++++++++++++++++\n");
    
    root.removeChild(p2);

    td.dumpTree(tn);
    System.out.println("removed p2 from root ++++++++++++++++++++++\n");

    p1.removeChild(p2);

    td.dumpTree(tn);
    System.out.println("removed p2 from p1 (now disposing entity?) ++++++++++++++++++++++\n");

    p1.addChild(p3);

    td.dumpTree(tn);
    System.out.println("added p3 to p1 ++++++++++++++++++++++\n");

    p2.addChild(p1);
    
    td.dumpTree(tn);
    System.out.println("added p1 to p2 (p2 not in tree) +++++++++++++\n");

    root.addChild(p2);

    td.dumpTree(tn);
    System.out.println("added p2 subtree ++++++++++++++++++++++\n");
  }
}
