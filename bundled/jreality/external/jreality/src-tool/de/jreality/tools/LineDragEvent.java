package de.jreality.tools;

import java.util.EventObject;

import de.jreality.scene.IndexedLineSet;
import de.jreality.scene.data.Attribute;

public class LineDragEvent extends EventObject {
    
	private static final long serialVersionUID = 19823L;

    private final int index;
    private final double[] translation;
    private final double[] position;
	private final IndexedLineSet lineSet;
  
	public LineDragEvent(IndexedLineSet lineSet, int index, double[] translation,double[] position) {
		super(lineSet);
    this.lineSet=lineSet;
    this.index=index;
    this.translation = (double[])translation.clone();
    this.position  = (double[])position.clone();
	}
	
	/** The x-coordinate of this event's translation. */
	public double getX() {
		return translation[0];
	}
	
	/** The y-coordinate of this event's translation. */
	public double getY() {
		return translation[1];
	}
	
	/** The z-coordinate of this event's translation. */
	public double getZ() {
		return translation[2];
	}

  public double[] getTranslation() {
    return (double[]) translation.clone();
  }
  
  public double[] getPosition() {
      return (double[]) position.clone();
    }
  
  public int getIndex() {
	  return index;
  }

  /**
   * BE CAREFUL: this method uses the line index when the drag started. So it makes only sense to use it
   * when the combinatorics of the indexed line set was not changed while dragging.
   * @return an array containing the indices of the line vertices
   * @throws ArrayIndexOutOfBoundsException
   */
  public int[] getLineIndices() {
	  return lineSet.getEdgeAttributes(Attribute.INDICES).toIntArrayArray().getValueAt(index).toIntArray(null);
  }  
  
  /**
   * BE CAREFUL: this method uses the line index when the drag started. So it makes only sense to use it
   * when the combinatorics of the indexed line set was not changed while dragging.
   * @return an array containing the line vertices
   * @throws ArrayIndexOutOfBoundsException
   */
  public double[][] getLineVertices(){
	    int[] lineIndices = getLineIndices();
		double[][] lineVertices=new double[lineIndices.length][];
		for(int i=0;i<lineIndices.length;i++)
			lineVertices[i]=lineSet.getVertexAttributes(Attribute.COORDINATES).toDoubleArrayArray().getValueAt(lineIndices[i]).toDoubleArray(null);
	  return lineVertices;
  }  
  public IndexedLineSet getIndexedLineSet() {
    return lineSet;
  }
}