package de.jreality.reader;

import java.util.concurrent.ArrayBlockingQueue;

class PointOctree {

  // the root, start searches here
  private Node root;

  private int size;


  // ********************************************************************************
  class Node {
    
    // the 3D-coords of this node
    private double x, y, z;

    private int index;

    // reference to the cildren 
    private Node[] children;
    
    public Node (double x, double y, double z) {
      this.children = new Node[8];
      this.index =0;
      this.x = x;
      this.y = y;
      this.z = z;
    }

    public Node (double x, double y, double z, int node_index) {
      this(x, y, z);
      index = node_index;
    }

    public boolean equals(Node n) { return this.equals(n.x, n.y, n.z); }

    public int index() { return index; }

    public void setIndex(int index) { this.index = index; }

    public Node(Node n) { this (n.x, n.y, n.z); }
    
    public boolean equals(double x, double y ,double z) {
      return (x == this.x && 
	      y == this.y && 
	      z == this.z);
    }

  }
  // ********************************************************************************

  public PointOctree() {
    size = 0;
    root = null;
  }

  public int size() { return this.size; }
 
  
  /** Is the current object empty? */
  public boolean isEmpty() {
    return (this.root == null) ? true : false;
  }

  /** 
   * Inserts a new node into the tree -- returns null if node is already existent, otherwise returns
   * a reference to the newly inserted element.
   **/
  public Node insert (double x , double y, double z) {

    // insert as root and return it
    if (this.isEmpty()) {
      root = new Node (x,y,z);
      this.size++;
      return root;
    }
    
    Node node = this.root;

    while (true) {
      if (node.equals( x,y,z)) return null;

      int sector = 0;
      if (x < node.x) sector += 4;
      if (y < node.y) sector += 2;
      if (z < node.z) sector += 1;
      
      if (node.children[sector] == null) {
	node.children[sector] = new Node( x, y, z, this.size++ );
	return node;
      }
      node = node.children[sector];
    }
  }


  /** Finds a node specified by its position keys, if not found returns the null referece. **/
  public Node find (double x , double y, double z) {
    
    Node node = this.root;
    if (node == null) return null;

    while (true) {
      if (node.equals( x,y,z)) return node;

      int sector = 0;
      if (x < node.x) sector += 4;
      if (y < node.y) sector += 2;
      if (z < node.z) sector += 1;
      
      if (node.children[sector] == null) {
	return null;
      }
      node = node.children[sector];
    }
  }

  /** Dumps the contents of the tree into an array. 
      @return reference to an array with all containing nodes, null if tree is
      empty. 
  **/
     
  public Node[] dump() throws Exception {

    if ( this.size == 0 ) return null;
    
    int cnt =0;
    Node[] nodeArray = new Node[ this.size ];
    ArrayBlockingQueue<Node> queue = new ArrayBlockingQueue<Node>( this.size );
    
    queue.add( this.root );
    
    while (!queue.isEmpty()) {
      Node n = queue.poll();
      nodeArray[cnt++] = n;
      for (int i=0; i < 8; ++i) {
	if (n.children[i] != null)
	  queue.add (n.children[i]);
      }
    }
    if (cnt != this.size) throw new Exception("Error: dumped not all nodes!");
    return nodeArray;
  }
}