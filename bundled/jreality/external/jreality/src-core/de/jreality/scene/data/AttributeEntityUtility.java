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


package de.jreality.scene.data;

import java.beans.BeanInfo;
import java.beans.IntrospectionException;
import java.beans.Introspector;
import java.beans.PropertyDescriptor;
import java.lang.reflect.Constructor;
import java.lang.reflect.Field;
import java.lang.reflect.InvocationHandler;
import java.lang.reflect.Method;
import java.lang.reflect.Proxy;
import java.text.MessageFormat;
import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.logging.Level;

import de.jreality.scene.Appearance;
import de.jreality.shader.EffectiveAppearance;
import de.jreality.shader.ShaderUtility;
import de.jreality.util.LoggingSystem;

/**
 * 
 * This class handles a whole set of Attributes, that belong together somehow. A
 * typical application is i.e. a Texture object, that consists of many
 * attributes, that should be set and get from an appearance as one entity.
 * 
 * Define the entity by a an interface that consists of set/get method pairs -
 * all these attributes will be handled by the Reader/Writer proxy.
 * 
 * Use this interface either as a Writer on an Appearance or a set of
 * Appearances:
 * 
 * <code>
 * <b><pre>
 * Appearance app = new Appearance();
 * MyEntityInterface mif = (MyEntityInterface) AttributeEntityFactory
 *     .createAttributeEntity(MyEntityInterface.class, &quot;myEntityName&quot;, app);
 * mif.setAttribute1(value1);
 * mif.setAttribute2(value2);
 * </pre></b>
 * </code>
 * 
 * And use it to read the values from an EffectiveAppearance: <code>
 * <b><pre>
 * 
 *  EffectiveAppearance ea = ...
 *  MyEntityInterface mif = (MyEntityInterface) AttributeEntityFactory.createAttributeEntity(MyEntityInterface.class, &quot;myEntityName&quot;, ea);
 *  value1 = mif.getAttribute1();
 *  value2 = mif.getAttribute2();
 *  
 * </pre></b>
 * </code>
 * <p>
 * <b>Note: Instances that act on {@link Appearance}s can call set- and
 * get-Methods, instances acting on {@link EffectiveAppearance}s can ONLY use
 * get-Methods. </b>
 * <p>
 * 
 * The given prefix is used as a name prefix for the single attributes. i.e. in
 * the above example the attribute "attribute1" is stored in the Appearance(s)
 * as setAttribute("myEntityName:attribute1");
 * 
 * To read the values again one needs to use the same prefix as used while
 * writing.
 * 
 * <p>
 * 
 * <h1>Naming conventions</h1>
 * <li>Attributes are named just like the methods without the prefix set or get
 * and with a small first character (not really, its simply javaBeans naming
 * convetions). i.e. set/getTestDouble -> testDouble</li>
 * <li>Default values are defined as constants in the interface. the name for a
 * default constant is defined as follows: <br>
 * every uppercase letter L that follows a number or a lowercase letter is
 * replaced by _L. Then the resulting string is converted to upper case. then
 * _DEFAULT is appended. <br>
 * examples: <br>
 * testDouble -> TEST_DOUBLE_DEFAULT <br>
 * testURL -> TEST_URL_DEFAULT <br>
 * 23AttrTest42String -> 23_ATTR_TEST42_STRING_DEFAULT <br>
 * 
 * TODO: possibly change name
 * 
 * @author weissman
 */
public class AttributeEntityUtility {

  private static class Handler implements InvocationHandler {

    private final Appearance app;
    private final EffectiveAppearance effApp;
    private final String ifClassName, prefix;
    private final HashMap<Method, PropertyDescriptor> readMethod, writeMethod, subReader, subCreator;
    private final PropertyDescriptor[] properties;
    private final boolean readDefaults;
    private final String seperator;
    
    Handler(Class ifClass,
    		PropertyDescriptor[] pd,
    		String prefix,
        HashMap<Method, PropertyDescriptor> reader,
        HashMap<Method, PropertyDescriptor> writer,
        HashMap<Method, PropertyDescriptor> subReader,
        HashMap<Method, PropertyDescriptor> subCreator,
        Object target,
        boolean readDefaults) {
      ifClassName = ifClass.getName();
      properties = pd;
      this.seperator = AttributeCollection.class.isAssignableFrom(ifClass) ? "." : ":";
      this.prefix = prefix == null || prefix.length() == 0 ? "" : prefix + seperator;
      readMethod = reader;
      writeMethod = writer;
      this.subReader = subReader;
      this.subCreator = subCreator;
      this.readDefaults = readDefaults;
      if (target instanceof EffectiveAppearance) {
        effApp = (EffectiveAppearance) target;
        app = null;
      } else {
        app = (Appearance) target;
        effApp = null;
      }
    }

    private Object getAttribute(PropertyDescriptor pd, Object proxy)
        throws IllegalAccessException {
      Object result = Appearance.INHERITED, defValue;
      String attrName = pd.getName();
      Class attrType = pd.getPropertyType();
      if (AttributeEntity.class.isAssignableFrom(attrType)) attrType = Class.class;
      if (attrType.isPrimitive()) attrType = wrapperType(attrType);
      if (app != null) result = app.getAttribute(prefix + attrName, attrType);
      else result = effApp.getAttribute(prefix + attrName, Appearance.INHERITED);
      if (result != Appearance.INHERITED && result != Appearance.DEFAULT)
        return result;
      if (!readDefaults) return null;
      // now read default value
      Object defaultVal = pd.getValue("default");
      if (defaultVal instanceof Class) {
        return defaultVal;
      }
      Field f = (Field) defaultVal;
      if (f != null) { 
        defValue = f.get(proxy);
      } else {
        if (attrType.isPrimitive()) {
          throw new IllegalStateException(MessageFormat.format(
              "no default value for primitive attribute {0}."
                  + "\npublic static final {1} {2} = <defaultValue>;",
              new Object[] { attrName, attrType, defaultFieldName(attrName) }));
        } else {
          defValue = null;
        }
      }
      if (effApp != null)
        result = effApp.getAttribute(prefix + attrName, defValue, attrType);
      else
        result = defValue;
      return result;
    }

    // TODO: no idea what exactly this should do - we need to specify what the
    // entities should do and how to specify defaults...
    private Object getSubEntityAttribute(PropertyDescriptor pd, Object proxy)
        throws IllegalAccessException {
      Object result = Appearance.INHERITED;
      String attrName = pd.getName();
      Class attrType = Class.class;
      
      if (app != null)
          result = app.getAttribute(prefix + attrName, attrType);
      if (result == Appearance.INHERITED && effApp != null)
          result = effApp.getAttribute(prefix + attrName, result, attrType);
      if (result != Appearance.INHERITED && result != Appearance.DEFAULT)
        return result;
      // check for a default value of the sub-entity (how is this specified?)
      Object defaultVal = pd.getValue("default");
      if (defaultVal instanceof Class) {
        try {
          // check for a default implementation of the sub-entities type.
          Field f = ((Class)defaultVal).getDeclaredField("CREATE_DEFAULT");
          return defaultVal;
        } catch (NoSuchFieldException nfe) {
        }
      }
      return null;
      
//      // if no default value for the sub-entity is given, it is a field (makes more sense: DEFAULT_POLYGON_SHADER)
//      Field f = (Field) defaultVal;
//      if (f != null)
//        defValue = f.get(proxy);
//      if (defValue instanceof Class) return defValue;
//      if (effApp != null)
//        result = effApp.getAttribute(prefix + attrName, defValue, attrType);
//      return result;
    }
    
    public Object invoke(Object proxy, Method method, Object[] args)
        throws Throwable {
      PropertyDescriptor pd = (PropertyDescriptor) readMethod.get(method);
      if (pd != null) {
        Object ret = getAttribute(pd, proxy);
        return ret;
      }
      pd = (PropertyDescriptor) writeMethod.get(method);
      if (pd != null) {
        if (app == null) throw new IllegalStateException("writing not allowed");
        String attrName = pd.getName();
        Class attrType = pd.getPropertyType();
        if (attrType.isPrimitive()) attrType = args[0].getClass();
        Object arg = args[0] == null ? Appearance.INHERITED : args[0];
        app.setAttribute(prefix + attrName, arg, attrType);
        return null;
      }
      pd = (PropertyDescriptor) subReader.get(method);
      if (pd != null) {
        return subEntity(pd, proxy);
      }
      if (app != null) {
        pd = (PropertyDescriptor) subCreator.get(method);
        if (pd != null) {
          String attrName = pd.getName();
          Class type;
          if (args == null || args.length == 0) type = pd.getPropertyType(); 
          else type = ShaderUtility.resolveEntity(pd.getPropertyType(), (String) args[0]);
          app.setAttribute(prefix + attrName, type, Class.class);
          return subEntity(pd, proxy);
        }
      }
      if (isHashCode(method)) {
        return new Integer(hashCode());
      }
      if (isEquals(method)) {
        return Boolean.valueOf(args[0] == proxy);
      }
      if (isToString(method)) {
        return toString(proxy);
      }
      throw new IllegalStateException("unhandled method " + method+"app==null ? "+(app == null));
    }

    private Object subEntity(PropertyDescriptor pd, Object proxy) throws IllegalAccessException {
      Class type = (Class) getSubEntityAttribute(pd, proxy);
      if (type == null) return null;
      String pref = prefix+pd.getName();
      if (app != null) return AttributeEntityUtility.getAttributeEntity(type, pref, app, readDefaults);
      if (AttributeEntityUtility.hasAttributeEntity(type, pref, effApp)) {
        try {
          return AttributeEntityUtility.createProxy(AttributeEntityUtility.resolveType(type), pref, effApp, true);
        } catch (IntrospectionException e) {
          throw new Error();
        }
      }
      return null;
    }

    private boolean isHashCode(Method method) {
      if (hashCode != null) return method.equals(hashCode);
      return method.getName() == "hashCode" && method.getDeclaringClass() == Object.class;
    }
    private boolean isEquals(Method method) {
      if (equals != null) return method.equals(equals);
      return method.getName() == "equals" && method.getDeclaringClass() == Object.class;
    }
    private boolean isToString(Method method) {
      if (toString != null) return method.equals(toString);
      return method.getName() == "toString" && method.getDeclaringClass() == Object.class;
    }

    String toString(Object proxy) {
      StringBuffer sb = new StringBuffer(2000);
      sb.append(ifClassName).append('[');
      sb.append("prefix = ").append(prefix);
      if (properties.length == 0) return sb.append(" ]").toString();
      sb.append(", ");
      int linePos = sb.length();
      for (int ix = 0; ix < properties.length; ix++) {
        PropertyDescriptor pd = properties[ix];
        int insPos = sb.length();
        sb.append(' ').append(pd.getDisplayName()).append(" = ");
        try {
          sb.append(getAttribute(pd, proxy));
        } catch (IllegalAccessException e) {
        }
        sb.append(", ");
        linePos += sb.length() - insPos;
        if (linePos > 60) {
          sb.setCharAt(sb.length() - 1, '\n');
          linePos = 0;
        }
      }
      sb.setCharAt(sb.length() - 2, ' ');
      sb.setCharAt(sb.length() - 1, ']');
      return sb.toString();
    }
  }

  private static Method equals;
  private static Method hashCode;
  private static Method toString;
  
  private static final HashMap<Class, PropertyDescriptor[]> descriptors=new HashMap<Class, PropertyDescriptor[]>();
  private static final HashMap<Class, HashMap<String, PropertyDescriptor>> properties=new HashMap<Class, HashMap<String,PropertyDescriptor>>();
  private static final HashMap<Class, HashMap<Method, PropertyDescriptor>> readers=new HashMap<Class, HashMap<Method, PropertyDescriptor>>();
  private static final HashMap<Class, HashMap<Method, PropertyDescriptor>> writers=new HashMap<Class, HashMap<Method, PropertyDescriptor>>();
  private static final HashMap<Class, Constructor<?>> proxyConstructors=new HashMap<Class, Constructor<?>>();
  private static HashMap<Class, HashMap<Method, PropertyDescriptor>> subEntityReaders=new HashMap<Class, HashMap<Method, PropertyDescriptor>>();
  private static HashMap<Class, HashMap<Method, PropertyDescriptor>> subEntityCreators=new HashMap<Class, HashMap<Method, PropertyDescriptor>>();

  static {
    try {
      try {
        hashCode = Object.class.getMethod("hashCode");
        equals = Object.class.getMethod("equals", Object.class);
        toString = Object.class.getMethod("toString");
      } catch (SecurityException se) {
        //hash
      }
    } catch (Exception e) {
      throw new ExceptionInInitializerError(e);
    }
  }

  /**
   * Create an implementation of the {@link AttributeEntity}for reading from
   * and/or writing to an {@link Appearance} - writes tag to the appearance.
   */
  public static AttributeEntity createAttributeEntity(Class clazz, String prefix,
      Appearance a, boolean readDefaults) {
      AttributeEntity proxy =  getAttributeEntity(clazz, prefix, a, readDefaults);
      if (!hasAttributeEntity(clazz, prefix, a)) {
        // tag the appearance
        a.setAttribute(getTaggingPrefix(prefix, clazz), clazz);
      }
      return proxy;
  }
  
  /**
   * Create an implementation of the {@link AttributeEntity}for reading from
   * and/or writing to an {@link Appearance} - this does not tag the appearance.
   */
  public static AttributeEntity getAttributeEntity(Class clazz, String prefix,
      Appearance a, boolean readDefaults) {
    if (prefix == null) prefix = "";
    try {
      AttributeEntity proxy = (AttributeEntity) createProxy(clazz, prefix, a, readDefaults);
      return proxy;
    } catch (IntrospectionException e) {
      IllegalStateException ise = new IllegalStateException(e.getMessage());
      ise.initCause(e);
      throw ise;
    }
  }

  /**
   * Create an implementation of the {@link AttributeEntity}for reading from an
   * {@link EffectiveAppearance}.
   */
  public static AttributeEntity createAttributeEntity(Class clazz, String prefix,
      EffectiveAppearance ea) {
    if (prefix == null) prefix = "";
    try {
      if (!hasAttributeEntity(clazz, prefix, ea))
        throw new IllegalStateException("no such entity");
      AttributeEntity proxy = (AttributeEntity) createProxy(resolveType(clazz), prefix, ea, true);
      return proxy;
    } catch (IntrospectionException e) {
      IllegalStateException ise = new IllegalStateException(e.getMessage());
      ise.initCause(e);
      throw ise;
    }
  }

  private static Class resolveType(Class clazz) {
    try {
      Field f = clazz.getDeclaredField("DEFAULT_ENTITY");
      return (Class) f.get(null);
    } catch (NoSuchFieldException nfe) {
    } catch (IllegalAccessException e) {
    }
    return clazz;
  }

  /**
   * returns true if the appearance is tagged with the given class or if
   * 1. clazz declares a DEFAULT_ENTITY class or
   * 2. clazz == DEFAULT_ENTITY
   * @param clazz
   * @param prefix
   * @param eap
   * @return
   */
  public static boolean hasAttributeEntity(Class clazz, String prefix, EffectiveAppearance eap) {
    if (hasAssignedAttributeEntity(clazz, prefix, eap)) return true;
    try {
      Field f = clazz.getField("DEFAULT_ENTITY");
      Class def = (Class) f.get(null);
      if (clazz == def || f.getDeclaringClass() == clazz) return true;
    } catch (NoSuchFieldException nfe) {
    } catch (IllegalAccessException e) {
      LoggingSystem.getLogger(AttributeEntityUtility.class).warning("error in default type declaration!");
    }
    return false;
  }
  
  public static boolean hasAttributeEntity(Class clazz, String prefix, Appearance a) {
    return a.getAttribute(getTaggingPrefix(prefix, clazz), Class.class) instanceof Class
    && clazz.isAssignableFrom((Class) a.getAttribute(getTaggingPrefix(prefix, clazz), Class.class));
  }
  
  private static boolean hasAssignedAttributeEntity(Class clazz, String prefix, EffectiveAppearance ea) {
    return clazz.isAssignableFrom((Class) ea.getAttribute(getTaggingPrefix(prefix, clazz), Object.class, Class.class));
  }
  
  private static String getTaggingPrefix(String prefix, Class clazz) {
    if (prefix == null || prefix.equals("")) return "<"+clazz.getName()+">";
    else return prefix;
  }

  private static Object createProxy(Class clazz, String prefix, Object target,
      /*boolean write,*/ boolean readDefaults) throws IntrospectionException {
    if (!descriptors.containsKey(clazz)) {
      LoggingSystem.getLogger(AttributeEntityUtility.class).log(Level.INFO,
          "creating reader {0} with prefix {1}", new Object[] { clazz, prefix });
      HashMap<String, PropertyDescriptor> prop = new HashMap<String, PropertyDescriptor>();
      HashMap<Method, PropertyDescriptor> reader = new HashMap<Method, PropertyDescriptor>(),
      	writer = new HashMap<Method, PropertyDescriptor>(),
      	subEntityReader = new HashMap<Method, PropertyDescriptor>(),
      	subEntityWriter = new HashMap<Method, PropertyDescriptor>();
      HashSet<PropertyDescriptor> pds = new HashSet<PropertyDescriptor>();
      BeanInfo bi = Introspector.getBeanInfo(clazz, Introspector.IGNORE_ALL_BEANINFO);
	  pds.addAll(Arrays.asList(bi.getPropertyDescriptors()));
	  for (Class<? extends AttributeEntity> type : clazz.getInterfaces()) {
    	  bi = Introspector.getBeanInfo(type, Introspector.IGNORE_ALL_BEANINFO);
    	  pds.addAll(Arrays.asList(bi.getPropertyDescriptors()));
      }
      for (PropertyDescriptor descriptor : pds) {
        String attrName = descriptor.getName();
        if (prop.put(attrName, descriptor) != null)
            throw new IllegalArgumentException("conflicts on property \""
                + attrName + '"');
        if (AttributeEntity.class.isAssignableFrom(descriptor.getPropertyType())) {
          // handle sub entities
          String subEntityName = Character.toUpperCase(attrName.charAt(0)) + attrName.substring(1);
          String createMethodName = "create" + subEntityName;
          Method creator = null;
          try {
            creator = clazz.getMethod(createMethodName, new Class[]{String.class});
          } catch (NoSuchMethodException nsme) {
            try {
              creator = clazz.getMethod(createMethodName);
            } catch (NoSuchMethodException e) {
            }
          }
          subEntityReader.put(descriptor.getReadMethod(), descriptor);
          if (creator != null) subEntityWriter.put(creator, descriptor);
          Class defaultClass = resolveType(descriptor.getPropertyType());
          descriptor.setValue("default", defaultClass);
        } else {
          reader.put(descriptor.getReadMethod(), descriptor);
          writer.put(descriptor.getWriteMethod(), descriptor);
          try {
            descriptor.setValue("default", clazz.getField(defaultFieldName(attrName)));
          } catch (NoSuchFieldException e) {
            Class type = descriptor.getPropertyType();
            int dim = 0;
            Level level = type.isPrimitive() ? Level.WARNING : Level.INFO;
            while (type.isArray()) {
              type = type.getComponentType();
              dim++;
            }
            String typeName = type.getName();
            for (int i = 0; i < dim; i++)
              typeName += "[]";
            LoggingSystem.getLogger(AttributeEntityUtility.class).log(
                level,
                "no default value for primitive attribute {0}."
                    + "\npublic static final {1} {2} = <defaultValue>;",
                new Object[] { attrName, typeName, defaultFieldName(attrName) });
          }
        }
      }

      Class proxyClass = Proxy.getProxyClass(clazz.getClassLoader(), clazz);
      descriptors.put(clazz, pds.toArray(new PropertyDescriptor[pds.size()]));
      properties.put(clazz, prop);
      readers.put(clazz, reader);
      writers.put(clazz, writer);
      subEntityReaders.put(clazz, subEntityReader);
      subEntityCreators.put(clazz, subEntityWriter);
      try {
        proxyConstructors.put(clazz, proxyClass.getConstructor(InvocationHandler.class));
      } catch (NoSuchMethodException e) {
        throw new Error();
      }
    }
    Handler h = new Handler(clazz,
        descriptors.get(clazz),
        prefix,
        readers.get(clazz),
        writers.get(clazz),
        subEntityReaders.get(clazz),
        subEntityCreators.get(clazz),
        target, readDefaults);
    try {
      return proxyConstructors.get(clazz).newInstance(h);
    } catch (Exception e) {
      e.printStackTrace();
      IllegalStateException ie = new IllegalStateException(e.getMessage());
      ie.initCause(e.getCause());
      throw ie;
    }
  }

  /**
   * converts an attribute name into the field name for its default value i.e.:
   * myCoolParameter -> MY_COOL_PARAMETER_DEFAULT
   * 
   * @param attrName
   *          the attribute name
   * @return default field name
   */
  private static String defaultFieldName(String attrName) {
    return attrName.replaceAll("([\\p{Ll},\\p{N}]+)([\\p{Lu}]{1})", "$1_$2")
        .toUpperCase()
        + "_DEFAULT";
  }

  private static Class wrapperType(Class attrType) {
    switch (attrType.getName().charAt(0)) {
      case 'd':
        return Double.class;
      case 'i':
        return Integer.class;
      case 'b':
        return attrType == Boolean.TYPE ? Boolean.class : Byte.class;
      case 'c':
        return Character.class;
      case 's':
        return Short.class;
      case 'l':
        return Long.class;
      case 'f':
        return Float.class;
      case 'v':
      default:
        throw new IllegalArgumentException(attrType.toString());
    }
  }

  private AttributeEntityUtility() {
  }
}
