package de.jreality;

import java.lang.reflect.AccessibleObject;
import java.lang.reflect.Field;
import java.lang.reflect.Modifier;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.IdentityHashMap;
import java.util.List;

/**
 * Some debug utilities.
 */
public class Debug
{
  private static HashMap<Class,Field[]> FIELDS=new HashMap<Class,Field[]>();
  private static Class STRING="".getClass();
  private Debug(){}
  public static void location(String message, Object param)
  {
    StackTraceElement[] ste=new Exception().getStackTrace();
    StackTraceElement caller=ste[1];
    if(message!=null) System.err.print(message+":  ");
    System.err.print(caller.getClassName());
    System.err.print('.');
    System.err.print(caller.getMethodName());
    System.err.print('(');
    if(param!=null) System.err.print(param);
    System.err.println(')');
    for(int i=2, n=Math.min(5, ste.length); i<n; i++)
      System.err.println(" + "+ste[i]);
  }
  public static void data(String message, Object object)
  {
//    StackTraceElement[] ste=new Exception().getStackTrace();
//    StackTraceElement caller=ste[1];
    if(message!=null) System.err.print(message+":  ");
    if(object==null)
    {
      System.err.println("null");
      return;
    }
    System.err.println(object.getClass().getName());
    try
    {
      data(object, 0, 1);
    } catch (IllegalAccessException e)
    {
      System.err.println(e);
    }
  }
  private static void data(Object object, int level, int maxLevel)
    throws IllegalAccessException
  {
    for(Class cl=object.getClass(); cl!=null; cl=cl.getSuperclass())
    {
      Field[] f=cl.getDeclaredFields();
      AccessibleObject.setAccessible(f, true);
      for(int i=0, n=f.length; i<n; i++)
      {
        Field ff=f[i];
        if(!Modifier.isStatic(ff.getModifiers()))
        {
          Class type=ff.getType();
          Object obj=ff.get(object);
          if(type.isPrimitive()||type==STRING||obj==null||type.getName().startsWith("java."))
          {
            System.err.print("       ".substring(0, level));
            System.err.print(ff.getName());
            System.err.print(" = ");
            System.err.println(obj);
          }
          else
          {
            System.err.print("       ".substring(0, level));
            System.err.print(ff.getName());
            System.err.println(" {");
            if(level<maxLevel) data(obj, level+1, maxLevel);
            System.err.print("       ".substring(0, level));
            System.err.println('}');
          }
        }
      }
    }
  }

  static final Object KNOWN=new Object();
  private static List<String> ref(Object object, Object ref, IdentityHashMap m)
    throws IllegalAccessException
  {
    if(object==null) return null;
    if(m==null) m=new IdentityHashMap();
    if(m.put(object, KNOWN)!=null) return null;
    for(Class cl=object.getClass(); cl!=null; cl=cl.getSuperclass())
    {
      Field[] f=FIELDS.get(cl);
      if(f==null)
      {
        f=cl.getDeclaredFields();
        AccessibleObject.setAccessible(f, true);
        FIELDS.put(cl, f);
      }
      for(int i=0, n=f.length; i<n; i++)
      {
        Field ff=f[i];
        if(ff.getType().isPrimitive()) continue;
        Object obj=ff.get(object);
        if(obj==ref)
        {
          List<String> l=new ArrayList<String>();
          l.add("\t"+ff.getDeclaringClass().getName()+"."+ff.getName()+" ["+ff.getType().getName()+"]");
          return l;
        }
        else if(obj instanceof Object[])
        {
          int cnt=0;
          for(Object el:(Object[])obj)
          {
        	List<String> l=ref(el, ref, m);
            if(l!=null) {
              String debugOutput="["+cnt+"]\t"
                +ff.getDeclaringClass().getName()+"."
                +ff.getName()+" ["
                +ff.getType().getName()+"]";
              l.add(debugOutput);
              return l;
            }
        	cnt++;
          }
            
        } else
        {
          List l=ref(obj, ref, m);
          if(l!=null) {
            l.add("\t"+ff.getDeclaringClass().getName()+"."+ff.getName()+" ["+ff.getType().getName()+"]");
            return l;
          }
        }
      }
    }
    return null;
  }
  public static void ref(Object object, Object ref)
  {
    try {
      List l=ref(object, ref, null);
      if(l!=null)
      {
        Collections.reverse(l);
        for(Object o:l)
        {
            System.out.println(o);
        }
        System.out.println();
      }
    } catch (IllegalAccessException e) {
      // TODO Auto-generated catch block
      e.printStackTrace();
    }
  }
}
