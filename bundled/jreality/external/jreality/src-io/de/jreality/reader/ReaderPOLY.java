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

import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.LineNumberReader;
import java.util.HashMap;
import java.util.Vector;

import de.jreality.geometry.IndexedFaceSetUtility;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.data.Attribute;
import de.jreality.scene.data.StorageModel;
import de.jreality.util.Input;
import de.jreality.util.LoggingSystem;


/**
 *
 * Simple parser for polymake files
 *
 * @author weissman
 *
 */
public class ReaderPOLY extends AbstractReader {

  public void setInput(Input input) throws IOException {
    super.setInput(input);
    root = parse(input.getInputStream());
  }
  
  static SceneGraphComponent parse(InputStream is) {
    InputStreamReader r = new InputStreamReader(is);
    LineNumberReader lr = new LineNumberReader(r);
    SceneGraphComponent root = new SceneGraphComponent();

    Vector v=null;
    HashMap map = new HashMap();
    
    String line; 
    try {
        while((line= lr.readLine()) !=null) {
            line = line.trim();
            if(line.equals("")) continue;
            if(Character.isUpperCase(line.charAt(0))) {
                LoggingSystem.getLogger(ReaderPOLY.class).finer(" make entry "+line);
                v = new Vector();
                map.put(line,v);
            } else if (v != null&& !line.equals("")) {
                v.add(line);
            }
        }
    } catch (IOException e) {
        // TODO Auto-generated catch block
        e.printStackTrace();
    }
    // now we have all the data but still mostly unparsed
    int o = 0;
    Vector vData = ((Vector)map.get("GEOMETRIC_REALIZATION"));
    int n = vData.size();
     double[] vertices = new double[3*n];
     for(int i = 0; i<n;i++) {
         String str = (String) vData.get(i);
         String[] vals = str.split("[\\s\\{\\}/]");
         LoggingSystem.getLogger(ReaderPOLY.class).finer("vals length "+vals.length);
         vertices[3*i  ] = Integer.parseInt(vals[0])/(double)Integer.parseInt(vals[1]);
         vertices[3*i+1] = Integer.parseInt(vals[2])/(double)Integer.parseInt(vals[3]);
         vertices[3*i+2] = Integer.parseInt(vals[4])/(double)Integer.parseInt(vals[5]);
     }
     
     vData = ((Vector)map.get("FACETS"));
     n = vData.size();
     int[][] faces = new int[n][3];
     for(int i = 0; i<n;i++) {
         String str = (String) vData.get(i);
         String[] vals = str.split("[\\s\\{\\}/]");
         LoggingSystem.getLogger(ReaderPOLY.class).finer("face vals length "+vals.length);
         if(vals.length>3 ) o = 1;
         else o = 0;
         faces[i][0] = Integer.parseInt(vals[o+0]);
         faces[i][1] = Integer.parseInt(vals[o+1]);
         faces[i][2] = Integer.parseInt(vals[o+2]);
     }
     
     IndexedFaceSet ifs = new IndexedFaceSet();
     
     ifs.setVertexCountAndAttributes(Attribute.COORDINATES,StorageModel.DOUBLE_ARRAY.inlined(3).createReadOnly(vertices));
     //ifs.setFaceCountAndAttributes(Attribute.INDICES,new IntArrayArray.Array(faces));
     ifs.setFaceCountAndAttributes(Attribute.INDICES,StorageModel.INT_ARRAY_ARRAY.createReadOnly(faces));
     IndexedFaceSetUtility.calculateAndSetFaceNormals(ifs);
     IndexedFaceSetUtility.calculateAndSetVertexNormals(ifs);
     IndexedFaceSetUtility.calculateAndSetEdgesFromFaces(ifs);
     root.setGeometry(ifs);
//     System.out.println("we return "+root+" with geometry "+root.getGeometry());
    return root;
}
}
