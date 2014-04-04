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


package de.jreality.util;

import java.net.URL;
import java.security.AllPermission;
import java.security.CodeSource;
import java.security.Permission;
import java.security.PermissionCollection;
import java.security.Permissions;
import java.security.Policy;
import java.util.Collection;
import java.util.Collections;
import java.util.HashSet;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.logging.Level;

/**
 * A policy which restricts access from a configured set of
 * code source {@link URL}s and grants access to all other.
 * The policy can be activated by calling <code>
 * {@link java.security.Policy#setPolicy(java.security.Policy)}
 * </code>.
 */
public class SimpleURLPolicy extends Policy
{
  private final static AllPermission ALL_PERM = new AllPermission(); 
  private final HashSet restricted = new HashSet();
  private final LinkedList restrictedPermissions=new LinkedList();
  
  public SimpleURLPolicy(Collection permissions, URL url) {
    this(permissions, Collections.singleton(url));
  }
  public SimpleURLPolicy(Collection permissions, Collection urls) {
    restrictedPermissions.addAll(permissions);
    restricted.addAll(urls);
    restricted.add(null);
  }
  public PermissionCollection getPermissions(CodeSource codesource)
  {
    Permissions pc=new Permissions();
    if(restricted.contains(codesource.getLocation()))
    {
      for (Iterator iter = restrictedPermissions.iterator(); iter.hasNext();) {
        Permission element = (Permission) iter.next();
        pc.add(element);
      }
    }
    else {
      pc.add(ALL_PERM);//unrestricted 
    }
    return pc;
  }

  public void refresh() {
    LoggingSystem.getLogger(this).log(Level.FINER, "refresh called");
  }
}
