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


package de.jreality.shader;

import java.util.Iterator;
import java.util.List;
import java.util.Vector;

import de.jreality.scene.Appearance;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SceneGraphNode;
import de.jreality.scene.SceneGraphPath;

/**
 * The attributes of {@link Appearance}s arer designed to be inherited via the
 * scene graph tree structure.  This class manages this inheritance mechanism.
 * <p>
 * To evaluate the actual state of the Appearance system at a point in a scene graph
 * specified by a {@link SceneGraphPath}, one must essentially create a chain of 
 * EffectiveAppearance's, one for each Appearance instance occurring on this path.
 * Requests for the value of a given key ({@link #getAttribute(String, Object)} and related
 * methods) cause a search "up" the tree towards the root, for an appearance which
 * containing this key.
 * <p>
 * If such an appearance is found, then the value is returned. 
 * If it is not found, the the special value {@link de.jreality.scene.Appearance#INHERITED}
 * is returned. 
 * <p>
 * TODO: The details of this inheritance mechanism are quite complicated and need to be 
 * further documented here.  In particular, the '.' character plays a special role in the
 * key strings, allowing for example the key <code>"foo"</code> to match a request for
 * the string <code>"bar.foo"</code>.
 * 
 */
public class EffectiveAppearance {
  @Override
	public String toString() {
		StringBuffer sb = new StringBuffer();
		EffectiveAppearance pea = this;
		do {
			sb.append(pea.app.getName()+":");
			pea = pea.parentApp;
		} while (pea != null);
		return sb.toString();
	}

private EffectiveAppearance parentApp;
  private Appearance app;

public Appearance getApp() {
	return app;
}
private EffectiveAppearance(EffectiveAppearance parent, Appearance app)
  {
    parentApp=parent;
    this.app=app;
  }
  public static EffectiveAppearance create()
  {
    return new EffectiveAppearance(null, new Appearance());
  }
  public static EffectiveAppearance create(SceneGraphPath p) {
    EffectiveAppearance eap = create();
    for (Iterator<SceneGraphNode> i = p.iterator(); i.hasNext(); ) {
      SceneGraphNode n = i.next();
      if (n instanceof SceneGraphComponent) {
        SceneGraphComponent sgc = (SceneGraphComponent) n;
        Appearance app = sgc.getAppearance();
        if (app != null) eap = eap.create(app);
      }
    }
    return eap;
  }
  
  public EffectiveAppearance create(Appearance app)
  {
    return new EffectiveAppearance(this, app);
  }
  
  public Object getAttribute(String key, Object defaultValue)
  {
    return getAttribute(key, defaultValue, defaultValue.getClass());
  }

  public Object getAttribute(String key, Object defaultValue, Class class1)
  {
    int lastDot=key.lastIndexOf('.');
    String lastKeyPart=key.substring(lastDot+1);
    for(int dot=lastDot; dot!=-1; dot=key.lastIndexOf('.', dot-1))
    {
      String localKey=key.substring(0, dot+1)+lastKeyPart;
      Object value = getAttribute1(localKey, defaultValue, class1);
      if(value!=Appearance.INHERITED) return value;
    }
    Object value = getAttribute1(lastKeyPart, defaultValue, class1);
    if(value==Appearance.INHERITED) value=defaultValue;
    return value;
  }
  private Object getAttribute1(String key, Object defaultValue, Class class1)
  {
    Object value = app.getAttribute(key, class1);
    if(value==Appearance.DEFAULT) return defaultValue;
    if(value!=Appearance.INHERITED) return value; // null not allowed
    return parentApp==null? Appearance.INHERITED:
      parentApp.getAttribute1(key, defaultValue, class1);
  }
  public double getAttribute(String key, double value)
  {
    return ((Double)getAttribute(key, new Double(value))).doubleValue();
  }
  public float getAttribute(String key, float value)
  {
    return ((Float)getAttribute(key, new Float(value))).floatValue();
  }
  public int getAttribute(String key, int value)
  {
    return ((Integer)getAttribute(key, new Integer(value))).intValue();
  }
  public long getAttribute(String key, long value)
  {
    return ((Long)getAttribute(key, new Long(value))).longValue();
  }
  public boolean getAttribute(String key, boolean value)
  {
    return ((Boolean)getAttribute(key, Boolean.valueOf(value))).booleanValue();
  }
  public char getAttribute(String key, char value)
  {
    return ((Character)getAttribute(key, new Character(value))).charValue();
  }
  
  public static boolean matches(final EffectiveAppearance eap, final SceneGraphPath p) {
    EffectiveAppearance ea=eap;
    for (Iterator<SceneGraphNode> li = p.reverseIterator(); li.hasNext(); ) {
      SceneGraphNode n = li.next();
      if (n instanceof SceneGraphComponent) {
        SceneGraphComponent sgc = (SceneGraphComponent) n;
        Appearance app = sgc.getAppearance();
        if (app != null) {
          if (ea.app != app) return false;
          ea = ea.parentApp; 
        }
      }
    }
    assert(ea != null); // must not happen since .create() 
                                  // creates a new Appearance that
                                  // nobody else can see
    if (ea.parentApp != null) {
      // eap has a non trivial prefix before path
      return false;
    }
    return true;
  }
  
  public List getAppearanceHierarchy()	{
	  Vector<Appearance> v = new Vector<Appearance>();
	  EffectiveAppearance pa = this;
	  if (pa.app != null) v.add(pa.app);
	  while ( (pa = pa.parentApp) != null)	{
		  if (pa.app != null) v.add(pa.app);
	  }
	  return v;
  }
}
