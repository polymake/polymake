package de.jreality.tools;



public final class FaceDragEventMulticaster implements FaceDragListener
{
  private final FaceDragListener a, b;
  private FaceDragEventMulticaster(FaceDragListener a, FaceDragListener b) {
      this.a = a; this.b = b;
  }
  private FaceDragListener remove(FaceDragListener oldl) {
    if(oldl == a)  return b;
    if(oldl == b)  return a;
    FaceDragListener a2 = remove(a, oldl);
    FaceDragListener b2 = remove(b, oldl);
    if(a2 == a && b2 == b) return this;
    return add(a2, b2);
  }
  public static FaceDragListener add(FaceDragListener a, FaceDragListener b)
  {
    final FaceDragListener result;
    if(a==null) result=b; else if(b==null) result=a;
    else result=new FaceDragEventMulticaster(a, b);
    return result;
  }
  public static FaceDragListener remove(FaceDragListener l, FaceDragListener oldl)
  {
    final FaceDragListener result;
    if(l==oldl||l==null) result=null;
    else if(l instanceof FaceDragEventMulticaster)
      result=((FaceDragEventMulticaster)l).remove(oldl);
    else result=l;
    return result;
  }

	public void faceDragStart(FaceDragEvent e) {
		a.faceDragStart(e); b.faceDragStart(e);
	}

	public void faceDragged(FaceDragEvent e) {
		a.faceDragged(e); b.faceDragged(e);
	}

	public void faceDragEnd(FaceDragEvent e) {
		a.faceDragEnd(e); b.faceDragEnd(e);
	}

}

