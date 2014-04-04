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

import java.awt.Color;
import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.Reader;
import java.io.StreamTokenizer;
import java.util.logging.Level;

import de.jreality.geometry.IndexedFaceSetFactory;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.util.Input;
import de.jreality.util.LoggingSystem;
import de.jreality.util.SceneGraphUtility;


/**
 *
 * A rudimentary reader for OOGL files (Geomview format). 
 * <p>
 * Needs to be converted to a real parser, e.g., by
 * using the antlr parsing package.
 * <p>
 * Current limitations: handles only following types: "OFF", "MESH", "VECT",  "LIST", "inst", "tlist".
 * @author Charles Gunn
 *
 */
public class ReaderMSMS extends AbstractReader {

  public void setInput(Input input) throws IOException {
    super.setInput(input);
    root = load(input.getInputStream());
  }
  
  /**
   * @param inputStream
   * @return
   */
  SceneGraphComponent load(InputStream inputStream) {
      Reader r = new BufferedReader(new InputStreamReader(inputStream));
       
      StreamTokenizer st = new StreamTokenizer(r);
      
      st.resetSyntax();
      st.eolIsSignificant(false);
      st.wordChars('0', '9');
      st.wordChars('A', 'Z');
      st.wordChars('a' , 'z');
      st.wordChars('.','.');
      st.wordChars('-','-');
      st.wordChars('+','+');
      st.wordChars('\u00A0', '\u00FF' );
      st.ordinaryChar('=');
      st.ordinaryChar('{');
      st.ordinaryChar('}');
      st.whitespaceChars('\u0000',  '\u0020');
      st.commentChar('#');
      //st.parseNumbers();
       
       SceneGraphComponent current= null;
      LoggingSystem.getLogger(ReaderMSMS.class).log(Level.FINER,"start.");
      try {
          current =SceneGraphUtility.createFullSceneGraphComponent("MSMS-node");
            //LoggingSystem.getLogger().log(Level.FINER,"found object!");
          int numV, numS;
          double density, probeRadius;
          st.nextToken();
          numV = Integer.parseInt(st.sval);
          st.nextToken();
          numS = Integer.parseInt(st.sval);
          st.nextToken();
          density = Double.parseDouble(st.sval);
          st.nextToken();
          probeRadius = Double.parseDouble(st.sval);
          double[][] verts = new double[numV][3],
          		normals = new double[numV][3];
          int[] nearestSphere = new int[numV], vertexType = new int[numV];
          for (int i=0; i<numV; ++i)  {
              for (int j = 0; j<3; ++j)  {
            	  st.nextToken();
            	  //LoggingSystem.getLogger().log(Level.FINER,"Token is "+st.sval);
            	  verts[i][j] = Double.parseDouble(st.sval);                        
              }
              for (int j = 0; j<3; ++j)  {
            	  st.nextToken();
            	  //LoggingSystem.getLogger().log(Level.FINER,"Token is "+st.sval);
            	  normals[i][j] = Double.parseDouble(st.sval);                        
              }
              st.nextToken();
              st.nextToken();
              nearestSphere[i] = Integer.parseInt(st.sval);
              st.nextToken();
              vertexType[i] = Integer.parseInt(st.sval);
          }
          // get the faces now
          int numF;
          st.nextToken();
          numF = Integer.parseInt(st.sval);
          // skip over redundant fields
          st.nextToken();
          st.nextToken();
          st.nextToken();
          int[][] indices = new int[numF][3];
          int[] faceType = new int[numF];
          Color[] facec = new Color[numF];
		   Color[] colors = {
		    		new Color(255,55,0), new Color(100,255,0), new Color(50,50,255),
		    		new Color(255,200,0), new Color(255,0,255), new Color(0,255,255)};
          for (int i = 0; i<numF; ++i)	{
        	  for (int j = 0; j<3; ++j)	{
        		  st.nextToken();
        		  indices[i][j] = Integer.parseInt(st.sval) - 1;
        	  }  
        	  st.nextToken();
        	  faceType[i] = Integer.parseInt(st.sval);
        	  facec[i] = colors[faceType[i] % colors.length];
        	  st.nextToken();
         }
          
          LoggingSystem.getLogger(ReaderMSMS.class).log(Level.INFO,"Read "+numV+" vertices and "+numF+" faces");
		IndexedFaceSetFactory ifsf = new IndexedFaceSetFactory();
		ifsf.setVertexCount(verts.length);
		ifsf.setVertexCoordinates(verts);
		ifsf.setVertexNormals(normals);
		ifsf.setFaceCount(indices.length);
		ifsf.setFaceIndices(indices);
//		ifsf.setFaceColors(facec);
		ifsf.setGenerateEdgesFromFaces(true);
		ifsf.setGenerateFaceNormals(true);
		ifsf.update();
          IndexedFaceSet ifs = ifsf.getIndexedFaceSet();
          ifs.setName("OFF Geometry");
          current.setGeometry(ifs);
        } catch (IOException e) {
          e.printStackTrace();
      }
      return current;
  }


}
