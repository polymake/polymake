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


package de.jreality.scene.data;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;

import junit.framework.TestCase;

public class StringStorageTest extends TestCase {

  private DataList sm;
  private StringArray sa;

  protected void setUp() throws Exception {
    String[] data={ "bla1", "bla2", "bla3", "bla4", "bla5" };
    sm = StorageModel.STRING_ARRAY.createReadOnly(data);    
    sa = (StringArray) sm;
  }
  
  public void testRead()
  {
    System.out.println(sm);
    for (int i = 0; i < sa.getLength(); i++)
      System.out.println(i+"="+sa.getValueAt(i));
  }

  public void testCopyOut() {
    String[] sarray = sa.toStringArray(null);
    for (int i = 0; i < sarray.length; i++)
      System.out.println(i+"="+sarray[i]);
  }

  public void testCopyIn() {
    Object cmp = checkStream(sm);
    String[] l2 = ((StringArray)cmp).toStringArray(null);
    assertEquals(sa.getLength(), l2.length);
    for (int i = 0; i < l2.length; i++)
      assertEquals(sa.getValueAt(i), l2[i]);
  }

  private Object checkStream(Object ifs) {
    try {
      ByteArrayOutputStream bos = new ByteArrayOutputStream(1024);
      ObjectOutputStream oos = new ObjectOutputStream(bos);
      oos.writeObject(ifs);
      oos.flush();
      oos.close();
      ObjectInputStream ois = new ObjectInputStream(new ByteArrayInputStream(bos.toByteArray()));
      Object read = ois.readObject();
      ois.close();
      return read;
    } catch (Exception e) {
      throw new Error(e);
    }
  }

}
