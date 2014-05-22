package de.jreality.reader;

import java.io.IOException;
import java.io.StreamTokenizer;
import java.util.ArrayList;
import java.util.Stack;

import de.jreality.geometry.IndexedFaceSetUtility;
import de.jreality.geometry.QuadMeshFactory;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.util.Input;

/**
 * simple reader for mathematica tables with entries{x,y,z,tx,ty} where the texture coordinates (tx,ty) are optional
 * 
 * @author weissman
 *
 */
public class ReaderMESH extends AbstractReader {


	Stack stack = new Stack();
	
	public void setInput(Input input) throws IOException {
		    super.setInput(input);
		    stack.push(new ArrayList());
		    load();
		    
		    System.out.println(stack.size());
		    ArrayList rows = (ArrayList) stack.peek();
		    rows = (ArrayList) rows.get(0);
		    int m=rows.size();
		    ArrayList col0 = (ArrayList) rows.get(0);
		    int n = col0.size();
		    
		    System.out.println("m="+m);
		    System.out.println("n="+n);
		    
		    double[][][] coords = new double[m][n][3];
		    double[][][] texCoords = new double[m][n][2];
		    for (int i=0; i<m; i++) {
		    	for (int j=0; j<n; j++) {
		    		ArrayList r = (ArrayList) rows.get(i);
		    		ArrayList p = (ArrayList) r.get(j);
		    		coords[i][j][0] = (Double) p.get(0);
		    		coords[i][j][1] = (Double) p.get(1);
		    		coords[i][j][2] = (Double) p.get(2);
		    		if (p.size() > 3) {
		    			texCoords[i][j][0] = (Double) p.get(3);
		    			texCoords[i][j][1] = (Double) p.get(4);
		    		}
		    	}
		    }
		    QuadMeshFactory qmf = new QuadMeshFactory();
		    qmf.setGenerateTextureCoordinates(false);
		    qmf.setVLineCount(m);
		    qmf.setULineCount(n);
		    qmf.setVertexCoordinates(coords);
		    qmf.setVertexTextureCoordinates(texCoords);
		    qmf.setGenerateFaceNormals(true);
		    qmf.setGenerateVertexNormals(true);
		    qmf.setGenerateEdgesFromFaces(true);
		    qmf.update();
		    IndexedFaceSet fs = qmf.getIndexedFaceSet();
		    IndexedFaceSetUtility.assignSmoothVertexNormals(fs, 6);
		    root=new SceneGraphComponent();
		    root.setGeometry(qmf.getIndexedFaceSet());
		  }

		  private StreamTokenizer globalSyntax(StreamTokenizer st) {
		    st.resetSyntax();
		    st.eolIsSignificant(false);
		    st.wordChars('{', '{');
		    st.wordChars('}', '}');
		    st.ordinaryChar('-');
		    st.wordChars('\u00A0', '\u00FF');
		    st.whitespaceChars('\u0000', '\u0020');
		    st.commentChar('#');
		    st.ordinaryChar('/');
		    st.parseNumbers();
		    return st;
		  }

		  private void load() throws IOException {
			    StreamTokenizer st = new StreamTokenizer(input.getReader());
			    globalSyntax(st);
			    while (st.nextToken() != StreamTokenizer.TT_EOF) {
			      if (st.ttype == StreamTokenizer.TT_WORD) {
			    	  String word = st.sval;
			    	  System.out.println("word="+st.sval);
			    	  loop: for (int i=0; i<word.length(); i++) {
			    		  if (word.charAt(i) == '{' ||  word.charAt(i) == '}') bracket(word.charAt(i));
			    		  else {
			    			number(Double.parseDouble(word.substring(i)));
			    			break loop;
			    		  }
			    	  }
			    	  
			      }if (st.ttype == StreamTokenizer.TT_NUMBER) {
			    	  System.out.println("number="+st.nval);
			    	  number(st.nval);
			      }
			    }
		  }

		private void bracket(char c) {
			if (c=='{') {
				ArrayList p = (ArrayList) stack.peek();
				ArrayList cl = new ArrayList();
				p.add(cl);
				stack.push(cl);
			} else {
				stack.pop();
			}
		}

		private void number(double nval) {
			ArrayList p = (ArrayList) stack.peek();
			p.add(nval);
		}

	
}
