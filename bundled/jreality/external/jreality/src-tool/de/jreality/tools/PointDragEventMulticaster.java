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


package de.jreality.tools;


public final class PointDragEventMulticaster implements PointDragListener
{
  private final PointDragListener a, b;
  private PointDragEventMulticaster(PointDragListener a, PointDragListener b) {
      this.a = a; this.b = b;
  }
  private PointDragListener remove(PointDragListener oldl) {
    if(oldl == a)  return b;
    if(oldl == b)  return a;
    PointDragListener a2 = remove(a, oldl);
    PointDragListener b2 = remove(b, oldl);
    if(a2 == a && b2 == b) return this;
    return add(a2, b2);
  }
  public static PointDragListener add(PointDragListener a, PointDragListener b)
  {
    final PointDragListener result;
    if(a==null) result=b; else if(b==null) result=a;
    else result=new PointDragEventMulticaster(a, b);
    return result;
  }
  public static PointDragListener remove(PointDragListener l, PointDragListener oldl)
  {
    final PointDragListener result;
    if(l==oldl||l==null) result=null;
    else if(l instanceof PointDragEventMulticaster)
      result=((PointDragEventMulticaster)l).remove(oldl);
    else result=l;
    return result;
  }

	public void pointDragStart(PointDragEvent e) {
		a.pointDragStart(e); b.pointDragStart(e);
	}

	public void pointDragged(PointDragEvent e) {
		a.pointDragged(e); b.pointDragged(e);
	}

	public void pointDragEnd(PointDragEvent e) {
		a.pointDragEnd(e); b.pointDragEnd(e);
	}

}
