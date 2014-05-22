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


package de.jreality.scene.proxy.smrj;

import java.util.Iterator;

import junit.framework.AssertionFailedError;
import junit.framework.TestCase;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.Lock;
import de.jreality.scene.data.Attribute;
import de.jreality.scene.data.ByteBufferList;
import de.jreality.scene.data.DataList;
import de.jreality.scene.data.DataListSet;
import de.smrj.Receiver;
import de.smrj.RemoteFactory;
import de.smrj.RemoteKey;

/**
 * 
 * THIS CLASS IS CURRENTLY NOT WORKING!!!
 * 
 * @author Steffen Weissmann
 *  
 */
public class SMRJMirrorFactoryTest extends TestCase {

    private static String HOST;
    private static RemoteFactory rf;
    private final static int localClients = 1;
    private final static Receiver[] rec = new Receiver[localClients*2]; 

    public static void main(String[] args) {
        junit.textui.TestRunner.run(SMRJMirrorFactoryTest.class);
    }

    SMRJMirrorScene proxyScene;
    
    IndexedFaceSet ch;
    
    public void testVisitIndexedFaceSet() throws Exception {
        proxyScene = new SMRJMirrorScene(rf, new Lock());
        ch = new IndexedFaceSet();
        ch.setName("my own proxy test node");
        Object proxy = proxyScene.createProxyScene(ch);
        //System.out.println("proxy: "+(Arrays.asList(proxy.getClass().getInterfaces())));
        //System.out.println(proxy);
        RemoteKey key = rf.getProxyKey(proxy);
        //System.out
        //        .println("visit: " + (System.currentTimeMillis() - l) + " ms");
        Thread.sleep(300);
        IndexedFaceSet copy = (IndexedFaceSet) rec[0].getClientFactory().getLocal(key);
        //System.out.println("orig: "+ch.getVertexAttributes());
        //System.out.println("copy: "+copy.getVertexAttributes());
        assertEquals(ch.getVertexAttributes(), copy.getVertexAttributes());
        assertEquals(ch.getEdgeAttributes(), copy.getEdgeAttributes());
        assertEquals(ch.getFaceAttributes(), copy.getFaceAttributes());
//        ch.setAlpha(1.2);
//        assertEquals(ch.getVertexAttributes(), copy.getVertexAttributes());
//        ch.setAlpha(1.4);
//        assertEquals(ch.getVertexAttributes(), copy.getVertexAttributes());
        for (int i =0; i < 10; i++) { 
        	System.out.println(i+". mal");
        	assertEquals(ch.getVertexAttributes(), copy.getVertexAttributes());
        }
    	IndexedFaceSet ifs = new IndexedFaceSet();
    	Object proxy2 = proxyScene.createProxyScene(ifs);
    	key = rf.getProxyKey(proxy2);
    	copy = (IndexedFaceSet) rec[0].getClientFactory().getLocal(key);
        for (int i = 201; i > 191; i--) {
        	IndexedFaceSet src= new IndexedFaceSet();
        	System.out.println(src.getVertexAttributes().storedAttributes());
        	ifs.setVertexCountAndAttributes(src.getVertexAttributes());
        	//System.out.println("SMRJMirrorFactoryTest.testVisitIndexedFaceSet() 1");
        	System.out.println(src.getEdgeAttributes().storedAttributes());
        	ifs.setEdgeCountAndAttributes(src.getEdgeAttributes());
        	//System.out.println("SMRJMirrorFactoryTest.testVisitIndexedFaceSet() 2");
        	//System.out.println(src.getFaceAttributes().storedAttributes());
        	ifs.setFaceCountAndAttributes(src.getFaceAttributes());
        	//System.out.println("SMRJMirrorFactoryTest.testVisitIndexedFaceSet() 3");
        	assertEquals(src.getVertexAttributes(), copy.getVertexAttributes());
        	assertEquals(src.getEdgeAttributes(), copy.getEdgeAttributes());
        	assertEquals(src.getFaceAttributes(), copy.getFaceAttributes());
        	ByteBufferList.BufferPool.releaseAll();
        }
    }

    public static void assertEquals(DataListSet arg0, DataListSet arg1) {
        assertEquals(arg0.storedAttributes(), arg1.storedAttributes());
        for (Iterator i = arg0.storedAttributes().iterator(); i.hasNext();) {
            Attribute a = (Attribute) i.next();
            assertEquals(arg0.getList(a), arg1.getList(a));
        }

    }

    public static void assertEquals(DataList arg0, DataList arg1) {
        assertEquals(arg0.getStorageModel(), arg1.getStorageModel());
        Object[] c0 = arg0.toArray();
        Object[] c1 = arg1.toArray();
        for (int i = 0; i < c0.length; i++)
            assertEquals(c0[i], c1[i]);
    }
    
    public static void assertEquals(double[] c0, double[] c1) {
        for (int i = 0; i < c0.length; i++)
            if (c0[i] != c1[i]) throw new AssertionFailedError("arrays differ "+c0[i]+"!="+c1[i]);
    }
}
