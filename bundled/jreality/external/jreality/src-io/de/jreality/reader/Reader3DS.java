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

import java.io.DataInputStream;
import java.io.EOFException;
import java.io.IOException;
import java.io.InputStream;
import java.util.Hashtable;
import java.util.logging.Level;
import java.util.logging.Logger;

import de.jreality.geometry.IndexedFaceSetUtility;
import de.jreality.math.MatrixBuilder;
import de.jreality.math.Rn;
import de.jreality.scene.Appearance;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.data.Attribute;
import de.jreality.scene.data.DoubleArrayArray;
import de.jreality.scene.data.IntArrayArray;
import de.jreality.scene.data.StorageModel;
import de.jreality.shader.CommonAttributes;
import de.jreality.shader.ImageData;
import de.jreality.shader.Texture2D;
import de.jreality.shader.TextureUtility;
import de.jreality.util.Input;

/**
 *
 * This class provides a reader for 3DS file format.
 *
 * @author weissman
 *
 */
public class Reader3DS extends AbstractReader {

  static final int ID_3DS_FILE = 0x4D4D;
  static final int ID_EDIT3DS = 0x3D3D;

  static final int ID_MATERIAL = 0xAFFF;
  static final int ID_MATERIAL_NAME = 0xA000;
  static final int ID_MATERIAL_AMBIENT = 0xA010;
  static final int ID_MATERIAL_DIFFUSE = 0xA020;
  static final int ID_MATERIAL_SPECULAR = 0xA030;
  static final int ID_MATERIAL_SHININESS = 0xA040;
  static final int ID_MATERIAL_TEXTURE = 0xA200;

  static final int ID_MATERIAL_TEXTUREFILE = 0xA300; // This holds the file name of the texture

  static final int ID_MAT_SHININESS_STRENGTH = 0xA041;
  static final int ID_MATERIAL_TRANSPARENCY = 0xA050;

  static final int ID_NAMED_OBJECT = 0x4000;
  static final int ID_TRIANGLE_SET = 0x4100;
  static final int ID_POINT_LIST = 0x4110;
  static final int ID_FACE_LIST = 0x4120;
  static final int ID_FACE_MATERIAL = 0x4130;
  static final int ID_MAPPING_COORDINATES_LIST = 0x4140;
  static final int ID_SMOOTH_GROUP = 0x4150;
  static final int ID_MESH_MATRIX = 0x4160;
  static final int ID_TEXTURE_COORDINATES = 0x4140;

  static final int ID_COLOR_FLOAT = 0x0010;
  static final int ID_COLOR_24 = 0x0011;
  static final int ID_COLOR_LIN_24 = 0x0012;
  static final int ID_COLOR_LIN_FLOAT = 0x0013;

  static final int ID_INT_PERCENTAGE = 0x0030;
  static final int ID_FLOAT_PERCENTAGE = 0x0031;

  private Appearance defaultAppearance = null;

  /**
   *  Constructor will set defaultAppearance and generate a root node
   */
  public Reader3DS() {
    setRootGroupNode(new SceneGraphComponent());
    setApperanceHash(new Hashtable());
    setCurrentSceneNode(null);
    setCurrentApperance(null);
    defaultAppearance = new Appearance();
  }

  private SceneGraphComponent mRootGroupNode;
  private double globalScale = 1;
  
  private void setRootGroupNode(SceneGraphComponent node) {
    mRootGroupNode = node;
  }

  public SceneGraphComponent getRootGroupNode() {
    return mRootGroupNode;
  }

  ///////////////////////////////////////////////
  //  Material Root Node
  ///////////////////////////////////////////////

  private Appearance mMaterialRootNode;
  Hashtable mAppearanceHash;

  private void setApperanceHash(Hashtable root) {
    mAppearanceHash = root;
  }

  private Hashtable getAppearanceHash() {
    return mAppearanceHash;
  }

  private void addAppearanceByName(Appearance matNode) {
    getAppearanceHash().put(matNode.getName(), matNode);
  }

  private Appearance getApperanceByName(String name) {
    if (name == null) return null;
    return (Appearance) getAppearanceHash().get(name);
  }

  ///////////////////////////////////////////////
  //  Current Node
  ///////////////////////////////////////////////

  private Appearance mCurrentApperance;
  private SceneGraphComponent mCurretSceneNode;
  private IndexedFaceSet mCurrentIFS;

  private void setCurrentApperance(Appearance node) {
    mCurrentApperance = node;
  }

  private Appearance getCurrentApperance() {
    return mCurrentApperance;
  }

  private void setCurrentSceneNode(SceneGraphComponent node) {
    mCurretSceneNode = node;
  }

  private SceneGraphComponent getCurrentSceneNode() {
    return mCurretSceneNode;
  }

  private void setCurrentIndexFaceSet(IndexedFaceSet ifs) {
    mCurrentIFS = ifs;
  }

  private IndexedFaceSet getCurrentIndexedFaceSet() {
    return mCurrentIFS;
  }

  ///////////////////////////////////////////////
  //  Load
  ///////////////////////////////////////////////

  private float color[] = new float[3];
  private float shininess;

  private boolean removeNode = false;

  private int lastPointCount = 0;

  public void setInput(Input input) throws IOException {
    SceneGraphComponent result = null;
    load(input.getInputStream());
    root = getRootGroupNode();
  }

  private boolean load(InputStream inputStream) {
    try {
      DataInputStream dataIn = new DataInputStream(inputStream);
      while (true) {
        int id = readUnsignedShort(dataIn);
        int len = readInt(dataIn);
        readChunk(dataIn, id, len);
      }
    } catch (EOFException eofe) {
    } catch (IOException ioe) {
      Logger.getLogger("de.jreality").log(Level.WARNING, "Parser3DS.load");
      Logger.getLogger("de.jreality").log(Level.WARNING, "\tIOException");
      return false;
    }

    return true;
  }

  private void readChunk(DataInputStream dataIn, int id, int len)
      throws IOException {
    switch (id) {
      case ID_3DS_FILE:
        Logger.getLogger("de.jreality").log(Level.FINER, "ID_3DS_FILE");
        return;
      case ID_EDIT3DS:
        Logger.getLogger("de.jreality").log(Level.FINER, "ID_EDIT3DS");
        break;

      //// MATERIAL /////////////////////////////////////////////
      case ID_MATERIAL: {
        Logger.getLogger("de.jreality").log(Level.FINER, "ID_MATERIAL");
        Appearance matNode = new Appearance();
        setCurrentApperance(matNode);
      }
        break;
      case ID_MATERIAL_NAME: // TODO: only named materials can be referenced..
      {
        Logger.getLogger("de.jreality").log(Level.FINER, "ID_MATERIAL_NAME");
        String matName = readString(dataIn);
        Logger.getLogger("de.jreality").log(Level.FINER, "\tname = " + matName);
        getCurrentApperance().setName(matName);
        addAppearanceByName(getCurrentApperance());
      }
        break;
      case ID_MATERIAL_AMBIENT: {
        readColor(dataIn, color);
        Logger.getLogger("de.jreality").log(
            Level.FINER,
            "ID_MATERIAL_AMBIENT = (" + color[0] + ", " + color[1] + ", "
                + color[2] + ")");
        getCurrentApperance().setAttribute(
            CommonAttributes.POLYGON_SHADER + "."
                + CommonAttributes.AMBIENT_COLOR,
            new java.awt.Color(color[0], color[1], color[2]));
      }
        break;
      case ID_MATERIAL_DIFFUSE: {
        readColor(dataIn, color);
        Logger.getLogger("de.jreality").log(
            Level.FINER,
            "ID_MATERIAL_DIFFUSE = (" + color[0] + ", " + color[1] + ", "
                + color[2] + ")");
        getCurrentApperance().setAttribute(
            CommonAttributes.POLYGON_SHADER + "."
                + CommonAttributes.DIFFUSE_COLOR,
            new java.awt.Color(color[0], color[1], color[2]));
      }
        break;
      case ID_MATERIAL_TEXTURE:
        break;
      case ID_MATERIAL_TEXTUREFILE: {
        String textName = readString(dataIn);
        if (textName.equals("")) break;
        Texture2D tex2d = null;
        try {
          ImageData imgData = ImageData.load(input.resolveInput(textName));
          if (imgData != null) {
            tex2d = TextureUtility.createTexture(getCurrentApperance(),
                CommonAttributes.POLYGON_SHADER,
                imgData, false);
            tex2d.setTextureMatrix(MatrixBuilder.euclidean().scale(1,-1,0).getMatrix());
          }
        } catch (Exception e) {
          // Just ignore invalid zextures
          Logger.getLogger("de.jreality").log(Level.WARNING,
              "Could not find the Texture " + textName);
          break;
        }
      }
      case ID_MATERIAL_SPECULAR: {
        readColor(dataIn, color);
        Logger.getLogger("de.jreality").log(
            Level.FINER,
            "ID_MATERIAL_SPECULAR = (" + color[0] + ", " + color[1] + ", "
                + color[2] + ")");
        getCurrentApperance().setAttribute(
            CommonAttributes.POLYGON_SHADER + "."
                + CommonAttributes.SPECULAR_COLOR,
            new java.awt.Color(color[0], color[1], color[2]));
      }
        break;
      case ID_MATERIAL_SHININESS: {
        shininess = readPercentage(dataIn);
        Logger.getLogger("de.jreality").log(Level.FINER,
            "ID_MATERIAL_SHININESS = " + shininess);
      }
        break;
      case ID_MAT_SHININESS_STRENGTH: {
        float shininessStrength = readPercentage(dataIn);
        Logger.getLogger("de.jreality").log(Level.FINER,
            "ID_MATERIAL_SHININESS_STRENGTH = " + shininessStrength);
        float matShiness = (1.0f - ((shininess + shininessStrength) / 2.0f)) * 128;
        Logger.getLogger("de.jreality").log(Level.FINER,
            "\tshininess = " + matShiness);
      }
        break;
      case ID_MATERIAL_TRANSPARENCY: // TODO: is there an appropiate way yet?!
      {
        float trans = readPercentage(dataIn);

        //                      getCurrentApperance().setAttribute(CommonAttributes.POLYGON_SHADER + "." + CommonAttributes.TRANSPARENT, true);
        //                      getCurrentApperance().setAttribute("transparency", trans);
      }
        break;

      //// OBJECT ///////////////////////////////////////////////

      /** This one ought to be an SceneGraphComponent */
      case ID_NAMED_OBJECT: {
        Logger.getLogger("de.jreality").log(Level.FINER, "ID_NAMED_OBJECT");
        String objName = readString(dataIn);
        Logger.getLogger("de.jreality").log(Level.FINER, "\tname = " + objName);
        SceneGraphComponent shapeNode = new SceneGraphComponent();
        shapeNode.setName(objName);
        getRootGroupNode().addChild(shapeNode);

        // simplify previous geometry
        if (getCurrentSceneNode() != null) {
          IndexedFaceSet ifs = (IndexedFaceSet) getCurrentSceneNode()
              .getGeometry();
          if (ifs != null) {
            getCurrentSceneNode().setGeometry(ifs);
          }
        }
        setCurrentSceneNode(shapeNode);
        defaultAppearance = new Appearance();
        shapeNode.setAppearance(defaultAppearance);
      }
        break;

      /** Place Geometry into Shape **/
      case ID_TRIANGLE_SET: {
        Logger.getLogger("de.jreality").log(Level.FINER, "ID_TRIANGLE_SET");
        IndexedFaceSet idxFaceSetNode = new IndexedFaceSet(0, 0);
        setCurrentIndexFaceSet(idxFaceSetNode);
        getCurrentSceneNode().setGeometry(idxFaceSetNode);
        removeNode = false;
        int[][] empty = new int[0][0];
        idxFaceSetNode.setFaceCountAndAttributes(Attribute.INDICES,
            new IntArrayArray.Array(empty));
      }
        break;

      case ID_TEXTURE_COORDINATES: {
        int nPoints = readUnsignedShort(dataIn);
        if (nPoints > 0) {
          IndexedFaceSet idxFaceSetNode = getCurrentIndexedFaceSet();
          double[][] texCoords = new double[nPoints][2];
          for (int n = 0; n < nPoints; n++) {
            double x = texCoords[n][0] = readFloat(dataIn);
            double y = texCoords[n][1] = readFloat(dataIn);
          }
          idxFaceSetNode.setVertexAttributes(Attribute.TEXTURE_COORDINATES,
              StorageModel.DOUBLE_ARRAY.array(2).createReadOnly(texCoords));
        }
      }
        break;

      case ID_POINT_LIST: {
        Logger.getLogger("de.jreality").log(Level.FINER, "ID_POINT_LIST");
        int nPoints = readUnsignedShort(dataIn);
        Logger.getLogger("de.jreality").log(Level.FINER,
            "\tnPoints = " + nPoints);
        lastPointCount = nPoints - 1;
        if (nPoints == 0) removeNode = true;
        if (nPoints > 0) {
          IndexedFaceSet idxFaceSetNode = getCurrentIndexedFaceSet();
          double[][] points = new double[nPoints][3];
          for (int n = 0; n < nPoints; n++) {
            double x = points[n][0] = readFloat(dataIn);
            double y = points[n][1] = readFloat(dataIn);
            double z = points[n][2] = readFloat(dataIn);
            //                              double y = points[n][2] = -readFloat(dataIn);
            //                          double z = points[n][1] = readFloat(dataIn);

            //Logger.getLogger("de.jreality").log(Level.FINER, "\t\t" + x + ", " + y + ", " + z);
            //System.out.println("\t\t" + x + ", " + y + ", " + z);
          }

          idxFaceSetNode.setVertexCountAndAttributes(Attribute.COORDINATES,
              StorageModel.DOUBLE_ARRAY.array(3).createReadOnly(points));
        }
      }
        break;
      case ID_FACE_LIST: {
        Logger.getLogger("de.jreality").log(Level.FINER, "ID_FACE_LIST");
        int nIndices = readUnsignedShort(dataIn);
        Logger.getLogger("de.jreality").log(Level.FINER,
            "\tnIndices = " + nIndices);
        if (nIndices > 0) {
          IndexedFaceSet idxFaceSetNode = getCurrentIndexedFaceSet();
          int[][] faces = new int[nIndices][3];

          double[][] faceNormals = new double[nIndices][3];
          double[][] verts = idxFaceSetNode.getVertexAttributes(
              Attribute.COORDINATES).toDoubleArrayArray(null);

          for (int n = 0; n < nIndices; n++) {
            int idx1 = faces[n][0] = readUnsignedShort(dataIn);
            int idx2 = faces[n][1] = readUnsignedShort(dataIn);
            int idx3 = faces[n][2] = readUnsignedShort(dataIn);

            double[] v1, v2;
            v1 = new double[3];
            v2 = new double[3];

            Rn.subtract(v1, verts[idx2], verts[idx3]);
            Rn.subtract(v2, verts[idx2], verts[idx1]);
            Rn.crossProduct(faceNormals[n], v1, v2);
            Rn.normalize(faceNormals[n], faceNormals[n]);

            if ((idx1 > lastPointCount) || (idx2 > lastPointCount)
                || (idx3 > lastPointCount)) {
              Logger.getLogger("de.jreality").log(Level.WARNING,
                  "reference to undefined vertex! removing node!");
              removeNode = true;
            }

            int idx4 = readUnsignedShort(dataIn);
            //                              Logger.getLogger("de.jreality").log(Level.FINER, "\t\t" + idx1 + ", " + idx2 + ", " + idx3);
          }
          idxFaceSetNode.setFaceCountAndAttributes(Attribute.INDICES,
              new IntArrayArray.Array(faces));
          idxFaceSetNode.setFaceAttributes(Attribute.NORMALS,
              new DoubleArrayArray.Array(faceNormals));

          /*
           idxFaceSetNode.setFaceAttributes(Attribute.NORMALS,
           new DataList(StorageModel.DOUBLE_ARRAY.array(3),
           GeometryUtility.calculateFaceNormals(idxFaceSetNode)));

           */
          /*
           idxFaceSetNode.setVertexAttributes(Attribute.NORMALS,
           new DataList(StorageModel.DOUBLE_ARRAY.array(3),
           GeometryUtility.calculateVertexNormals(idxFaceSetNode)));*/
          IndexedFaceSetUtility.calculateAndSetEdgesFromFaces(idxFaceSetNode);
        }
      }
        break;
      case ID_FACE_MATERIAL: {
        Logger.getLogger("de.jreality").log(Level.FINER, "ID_FACE_MATERIAL");
        String matName = readString(dataIn);
        Logger.getLogger("de.jreality").log(Level.FINER, "\tname = " + matName);
        Appearance appearance = getApperanceByName(matName);

        if (appearance != null) {
          getCurrentSceneNode().setAppearance(appearance);
        } else {
          getCurrentSceneNode().setAppearance(defaultAppearance);
        }

        if (removeNode) {
          getRootGroupNode().removeChild(getCurrentSceneNode());
          Logger.getLogger("de.jreality").log(Level.FINER,
              "\tremoved node " + getCurrentSceneNode().getName());
        }
        int nCounts = readUnsignedShort(dataIn);
        for (int n = 0; n < nCounts; n++)
          readUnsignedShort(dataIn);
      }
        break;
      //              case ID_MAPPING_COORDINATES_LIST:
      //                  {
      //                      Logger.getLogger("de.jreality").log(Level.FINER, "ID_MAPPING_COORDINATES_LIST");
      //                      int n = readUnsignedShort(dataIn);
      //                      double[] texCoords = new double[2*n];
      //                      for(int i = 0;i<n;i++) {
      //                          texCoords[2*i] =readFloat(dataIn);
      //                          texCoords[2*i+1] =readFloat(dataIn);
      //                      }
      //                      IndexedFaceSet idxFaceSetNode = getCurrentIndexedFaceSet();
      //                      idxFaceSetNode.setVertexAttributes(Attribute.TEXTURE_COORDINATES,
      //                              new DataList(StorageModel.DOUBLE_ARRAY.inlined(2), texCoords));
      //                      //skipChunk(dataIn, len);
      //                  }
      //                  break;
      default:
        Logger.getLogger("de.jreality").log(
            Level.FINE,
            "UNKOWN CHUNK! ID : 0x" + Integer.toHexString(id) + "  LEN : "
                + len);
        skipChunk(dataIn, len);
    }
  }

  ///////////////////////////////////////////////
  //  Read
  ///////////////////////////////////////////////

  private int readUnsignedByte(DataInputStream dataIn) throws IOException {
    return dataIn.readUnsignedByte();
  }

  private int readUnsignedShort(DataInputStream dataIn) throws IOException {
    int value = dataIn.readUnsignedShort();
    return ((value << 8) & 0xFF00) | ((value >> 8) & 0x00FF);
  }

  private int readInt(DataInputStream dataIn) throws IOException {
    int value = dataIn.readInt();
    return ((value << 24) & 0xFF000000) | ((value << 8) & 0x00FF0000)
        | ((value >> 8) & 0x0000FF00) | ((value >> 24) & 0x000000FF);
  }

  private float readFloat(DataInputStream dataIn) throws IOException {
    return Float.intBitsToFloat(readInt(dataIn));
  }

  private String readString(DataInputStream dataIn) throws IOException {
    StringBuffer sb = new StringBuffer();
    byte c = dataIn.readByte();
    while (c != (byte) 0) {
      sb.append((char) c);
      c = dataIn.readByte();
    }
    return sb.toString();
  }

  private void readColor(DataInputStream dataIn, float color[])
      throws IOException {
    int type = readUnsignedShort(dataIn);
    int len = readInt(dataIn);
    switch (type) {
      case ID_COLOR_FLOAT:
        color[0] = readFloat(dataIn);
        color[1] = readFloat(dataIn);
        color[2] = readFloat(dataIn);
        break;
      case ID_COLOR_24:
        color[0] = (float) readUnsignedByte(dataIn) / 255.0f;
        color[1] = (float) readUnsignedByte(dataIn) / 255.0f;
        color[2] = (float) readUnsignedByte(dataIn) / 255.0f;
        break;
      default:
        Logger.getLogger("de.jreality").log(Level.WARNING,
            "Unknown Color Type : 0x" + Integer.toHexString(type));
    }
  }

  private float readPercentage(DataInputStream dataIn) throws IOException {
    int type = readUnsignedShort(dataIn);
    int len = readInt(dataIn);
    switch (type) {
      case ID_INT_PERCENTAGE:
        return (float) readUnsignedShort(dataIn) / 100;
      case ID_FLOAT_PERCENTAGE:
        return readFloat(dataIn);
      default:
        Logger.getLogger("de.jreality").log(Level.WARNING,
            "Unknown Percentage Type : 0x" + Integer.toHexString(type));
    }
    return 0.0f;
  }

  private void skipChunk(DataInputStream dataIn, int len) throws IOException {
    int nSkipBytes = len - 6;
    if (0 < nSkipBytes) dataIn.skipBytes(nSkipBytes);
  }

  public void setGlobalScale(double globalScale) {
    this.globalScale = globalScale;
  }
}
