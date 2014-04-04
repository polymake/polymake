package de.jreality.soft;

import java.awt.Component;
import java.awt.EventQueue;

import javax.swing.JFrame;

import de.jreality.examples.CatenoidHelicoid;
import de.jreality.io.JrScene;
import de.jreality.io.JrSceneFactory;
import de.jreality.scene.SceneGraphComponent;

public class TestRenderRenderAsync {

  private static final int RENDER_CNT_SYNC = 50;
  private static final int RENDER_CNT_ASYNC = 100;

  static final DefaultViewer v = new DefaultViewer();
  
  public static void main(String[] args) throws Exception {
    JrScene scene = JrSceneFactory.getDefaultDesktopScene();
    v.setSceneRoot(scene.getSceneRoot());
    v.setCameraPath(scene.getPath("cameraPath"));
    SceneGraphComponent cmp = scene.getPath("emptyPickPath").getLastComponent();
    CatenoidHelicoid ch = new CatenoidHelicoid(10);
    cmp.setGeometry(ch);
    
    JFrame f = new JFrame();
    f.setSize(300, 300);
    Component vc = (Component) v.getViewingComponent();
    f.getContentPane().add(vc);
    f.setVisible(true);
    v.render();
    vc.validate();
    for (int i = 0; i < 10; i++) {
      final boolean synch = (i%2) == 0;
      Thread t = new Thread(new Runnable() {
        public void run() {
          TestRenderRenderAsync.run(synch);
        }
      }, "thread "+i);
      t.start();
      Thread.sleep(100);
    }
    Thread t = new Thread(new Runnable() {
      public void run() {
        for (int i = 0; i < RENDER_CNT_SYNC; i++) {
          try {
            EventQueue.invokeLater(new Runnable() {
              public void run(){
                synchronized (this) {
                  v.render();
                  System.out.println("rendered AWT");
                }
              }
            });
          } catch (Exception e) {
            e.printStackTrace();
          }
        }
      }
    }, "thread [AWT] ");
    t.start();

    while (true) {
      ch.setAlpha(ch.getAlpha()+0.001);
      Thread.sleep(1);
    }
  }

  private static void run(boolean synch) {
    if (!synch) {
      for (int i = 0; i < RENDER_CNT_ASYNC; i++) {
        v.renderAsync();
        Thread.yield();
      }
    } else {
      for (int i = 0; i < RENDER_CNT_SYNC; i++) {
        v.render();
        Thread.yield();
      }
    }
    System.out.println(Thread.currentThread().getName()+" FINISHED");
  }
}
