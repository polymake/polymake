package de.jreality.tools;



public final class LineDragEventMulticaster implements LineDragListener
{
  private final LineDragListener a, b;
  private LineDragEventMulticaster(LineDragListener a, LineDragListener b) {
      this.a = a; this.b = b;
  }
  private LineDragListener remove(LineDragListener oldl) {
    if(oldl == a)  return b;
    if(oldl == b)  return a;
    LineDragListener a2 = remove(a, oldl);
    LineDragListener b2 = remove(b, oldl);
    if(a2 == a && b2 == b) return this;
    return add(a2, b2);
  }
  public static LineDragListener add(LineDragListener a, LineDragListener b)
  {
    final LineDragListener result;
    if(a==null) result=b; else if(b==null) result=a;
    else result=new LineDragEventMulticaster(a, b);
    return result;
  }
  public static LineDragListener remove(LineDragListener l, LineDragListener oldl)
  {
    final LineDragListener result;
    if(l==oldl||l==null) result=null;
    else if(l instanceof LineDragEventMulticaster)
      result=((LineDragEventMulticaster)l).remove(oldl);
    else result=l;
    return result;
  }

	public void lineDragStart(LineDragEvent e) {
		a.lineDragStart(e); b.lineDragStart(e);
	}

	public void lineDragged(LineDragEvent e) {
		a.lineDragged(e); b.lineDragged(e);
	}

	public void lineDragEnd(LineDragEvent e) {
		a.lineDragEnd(e); b.lineDragEnd(e);
	}

}

