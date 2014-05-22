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


package de.jreality.reader;

import java.beans.Statement;
import java.io.IOException;
import java.io.PrintStream;
import java.io.Reader;
import java.net.URL;
import java.net.URLClassLoader;
import java.security.AllPermission;
import java.security.Permission;
import java.security.Policy;
import java.security.SecureClassLoader;
import java.util.LinkedList;
import java.util.logging.Level;

import de.jreality.scene.SceneGraphComponent;
import de.jreality.util.Input;
import de.jreality.util.LoggingSystem;
import de.jreality.util.Secure;
import de.jreality.util.SimpleURLPolicy;
import de.jreality.util.SystemProperties;

/**
 *
 * Reader for bsh script files. It creates an bsh.Interpreter 
 * instance, see www.beanshell.org for details. In this instance
 * there is a SceneGraphComponent registered as "root", to which the script
 * should attatch the scene parts. Example script:
 * 
 * <pre>
 * a=new Appearance();
 * a.setAttribute("plogonShader.diffuseColor", new java.awt.Color(77,66,44,72));
 * root.setAppearance(a);
 * root.setGeometry(new CatenoidHelicoid(10));
 * </pre>
 *
 * <b>For security reasons, this reader sets up a SecurityManager
 * and a restricting Policy for the codebase of the bsh.jar URL.
 * For this to work the bsh jar must NOT be in the classpath. 
 * Pass the location of the bsh.jar file as System property, i.e.:
 * <pre>
 * -Djreality.bsh.jar=file:///opt/jars/bsh-1.3b2.jar
 * </pre>
 * </b>
 * 
 * @author weissman
 *
 */
public class ReaderBSH implements SceneReader {

  private static final SecureClassLoader bshLoader;
  private static final SimpleURLPolicy bshPolicy;
  
  static {
    try {
      URL bshURL = new URL(Secure.getProperty(SystemProperties.BSH_JAR, "file:///net/MathVis/Oorange/oorange1.9/lib/bsh-1.3b2.jar"));
      bshLoader = new URLClassLoader(new URL[]{bshURL}, Thread.currentThread().getContextClassLoader());
      LinkedList<Permission> pc = new LinkedList<Permission>();
//      pc.add(new java.lang.RuntimePermission("getClassLoader"));
//      pc.add(new java.lang.RuntimePermission("accessDeclaredMembers"));
//      pc.add(new java.util.PropertyPermission("*", "read"));
//      pc.add(new java.io.FilePermission("./-", "read"));
//      pc.add(new java.net.SocketPermission("*:1024-", "listen,accept"));
      pc.add(new AllPermission());
      bshPolicy = new SimpleURLPolicy(pc, bshURL);
      Policy.setPolicy(bshPolicy);
    } catch (Exception e) {
      throw new ExceptionInInitializerError(e);
    }
  }
  
  private Object bsh;
  private SceneGraphComponent root;

  public ReaderBSH() throws Exception {
    System.setSecurityManager(new SecurityManager());
    root = new SceneGraphComponent();
    root.setName("BSHroot");
    bsh = bshLoader.loadClass("bsh.Interpreter").getConstructor(
        new Class[]{Reader.class, PrintStream.class, PrintStream.class, boolean.class}
        ).newInstance(new Object[]{null, System.out, System.err, Boolean.TRUE});
    exec(bsh, "eval", "import de.jreality.scene.*");
    exec(bsh, "eval", "import de.jreality.geometry.*");
    exec(bsh, "eval", "import de.jreality.scene.data.*");
    exec(bsh, "set", new Object[] { "root", root });
  }

  public void setInput(Input input) throws IOException {
    try {
      processReader(input.getReader(), null, null);
    } catch (Exception e) {
      LoggingSystem.getLogger(this).log(Level.SEVERE, "eval failed", e);
    }

  }

  /**
   * process the given Input in the current Instance.
   * @param input the Input to process
   * @throws IOException
   */
  public void appendInput(Input input) throws IOException {
    setInput(input);
  }

  public void processReader(Reader in, PrintStream out, PrintStream err) throws Exception {
    exec(bsh, "setOut", out);
    exec(bsh, "setErr", err);
    exec(bsh, "eval", in);
  }

  public SceneGraphComponent getComponent() {
    return root;
  }

  private static void exec(Object target, String methodName, Object arg) throws Exception {
    exec(target, methodName, new Object[]{arg});
  }

  private static void exec(Object target, String methodName, Object[] args) throws Exception {
    Statement s = new Statement(target, methodName, args);
    s.execute();
  }

}
