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

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.Reader;
import java.io.StreamTokenizer;

import de.jreality.geometry.IndexedFaceSetUtility;
import de.jreality.math.MatrixBuilder;
import de.jreality.scene.Cylinder;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.Transformation;
import de.jreality.scene.data.Attribute;
import de.jreality.scene.data.StorageModel;
import de.jreality.util.Input;
import de.jreality.util.LoggingSystem;

/**
 *
 * Simple POV reader. Note: The default is to use
 * Cylinders, see {@link ReaderPOV#setUseCylinders(boolean)}. 
 *
 * @author timh
 *
 */
public class ReaderPOV extends AbstractReader {

  private boolean useCylinders = false;
  private static int UNIT_DISK_DETAIL = 32;

  public void setUseCylinders(boolean useCylinders) {
    this.useCylinders = useCylinders;
  }
  
  public void setInput(Input input) throws IOException {
    super.setInput(input);
    root = load(input.getInputStream());
  }

  private SceneGraphComponent load(InputStream inputStream) {
    Reader r = new BufferedReader(new InputStreamReader(inputStream));
    SceneGraphComponent disk = new SceneGraphComponent();
    if (useCylinders) {
      disk.setGeometry(new Cylinder());
    } else {
      disk.setGeometry(new UnitDisk(UNIT_DISK_DETAIL));
    }
    
    MatrixBuilder.euclidean().scale(1,1,0.2).assignTo(disk);
    
    StreamTokenizer st = new StreamTokenizer(r);

    st.ordinaryChar('{');
    st.ordinaryChar('}');
    st.parseNumbers();

    SceneGraphComponent root = new SceneGraphComponent();
    SceneGraphComponent current = null;
    int bc = 0;
    int oc = 0;
    LoggingSystem.getLogger(this).fine("start.");
    try {
      while (st.ttype != StreamTokenizer.TT_EOF) {
        st.nextToken();
        if (st.ttype == StreamTokenizer.TT_WORD && st.sval.equals("object")) {
          while (st.ttype != '{')
            st.nextToken();
          oc = bc;
          current = new SceneGraphComponent();
        }
        if (st.ttype == '{') bc++;
        if (st.ttype == '}') bc--;
        if (bc == oc && current != null) {
          root.addChild(current);
          current = null;
        }
        if (st.ttype == StreamTokenizer.TT_WORD && st.sval.equals("Disk")
            && current != null) {
          current.addChild(disk);
        }

        if (st.ttype == StreamTokenizer.TT_WORD && st.sval.equals("matrix")
            && current != null) {
          current.setTransformation(readMatrix(st));
        }

      }
    } catch (IOException e) {
      e.printStackTrace();
    }
    LoggingSystem.getLogger(this).fine(
        "made " + root.getChildComponentCount() + " components");
    LoggingSystem.getLogger(this).fine("done.");
    return root;
  }

  private Transformation readMatrix(StreamTokenizer st) throws IOException {
    double[] d = new double[12];
    double[] m = new double[16];
    for (int i = 0; i < 12; i++) {
      int b = 0;
      while (st.ttype != StreamTokenizer.TT_NUMBER && b < 40) {
        st.nextToken();
        b++;
      }
      if (b == 40)
          LoggingSystem.getLogger(this).fine(
              "Error number " + i + " was aborted due to recursion.");
      d[i] = st.nval;
      st.nextToken();
      if (st.ttype == StreamTokenizer.TT_WORD && st.sval.startsWith("E")) {
        int exp = Integer.parseInt(st.sval.substring(1));
        d[i] *= Math.pow(10, exp);
        st.nextToken();
      }
    }
    for (int j = 0; j < 4; j++)
      for (int i = 0; i < 3; i++)
        m[j + 4 * i] = d[i + 3 * j];
    m[3 + 4 * 3] = 1;
    Transformation t = new Transformation();
    t.setMatrix(m);
    return t;
  }

  private static class UnitDisk extends IndexedFaceSet {

    public UnitDisk(int detail) {
      super();

      double r = 1;
      double[] vertices = new double[detail * 3 + 3];
      double[] normals = new double[detail * 3 + 3];
      //int[][] faces =new int[detail][3];
      int[] faces = new int[detail * 3];
      compute(vertices, normals, faces);

      setVertexCountAndAttributes(Attribute.COORDINATES,
          StorageModel.DOUBLE_ARRAY.inlined(3).createReadOnly(vertices));
      //setVertexAttributes(Attribute.NORMALS,
      //        StorageModel.DOUBLE_ARRAY.inlined(3).createReadOnly(normals));
      setFaceCountAndAttributes(Attribute.INDICES, StorageModel.INT_ARRAY
          .inlined(3).createReadOnly(faces));
      IndexedFaceSetUtility.calculateAndSetEdgesFromFaces(this);
      IndexedFaceSetUtility.calculateAndSetFaceNormals(this);
      IndexedFaceSetUtility.calculateAndSetVertexNormals(this);
    }

    private void compute(double[] vertices, double[] normals, int[] faces) {
      // The disk:
      int k = UNIT_DISK_DETAIL;
      for (int i = 0; i < k; i++) {

        faces[3 * i + 0] = (i);
        faces[3 * i + 1] = (((i + 1) % k));
        faces[3 * i + 2] = (k);

        double theta = 2 * Math.PI * i / k;
        double cosT = Math.cos(theta);
        double sinT = Math.sin(theta);

        int pos = 3 * (i);
        vertices[pos + 0] = cosT;
        vertices[pos + 1] = sinT;
        vertices[pos + 2] = 0;
        normals[pos + 0] = 0;
        normals[pos + 1] = 0;
        normals[pos + 2] = 1;
      }
      vertices[3 * (k)] = 0;
      vertices[3 * (k) + 1] = 0;
      vertices[3 * (k) + 2] = 0;

      normals[3 * (k)] = 0;
      normals[3 * (k) + 1] = 0;
      normals[3 * (k) + 2] = 1;

    }

  }

}
