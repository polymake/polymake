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


package de.jreality.util;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import java.util.Properties;
import java.util.StringTokenizer;
import java.util.logging.Level;
import java.util.logging.Logger;

import de.jreality.math.Matrix;

/**
 * This class is for managing configuration settings based on java.lang.Properties.
 * The config file used is given as a system property, namely {@link SystemProperties#CONFIG_SETTINGS}.
 * Currently only the de.jreality.portal-Package uses this class
 * 
 * The properties file is given via the system property {@link SystemProperties#CONFIG_SETTINGS},
 * if not it is assumed to be {@link SystemProperties#CONFIG_SETTINGS_DEFAULT} (in the current directory). 
 *
 *  @author weissman
 */
public class ConfigurationAttributes extends Properties {

    public static ConfigurationAttributes getDefaultConfiguration() {
        try {
            return new ConfigurationAttributes(
                Input.getInput(new File(Secure.getProperty(SystemProperties.CONFIG_SETTINGS, SystemProperties.CONFIG_SETTINGS_DEFAULT))),
                null);
        } catch (Exception e) {
            LoggingSystem.getLogger(ConfigurationAttributes.class).log(Level.WARNING, "loading default Configuration", e);
        }
        return new ConfigurationAttributes();
    }
    
    private ConfigurationAttributes() {}

    public ConfigurationAttributes(Input input) {
      init(input, null);
    }

    public ConfigurationAttributes(Input input, ConfigurationAttributes parent) {
        init(input, parent);
    }

    public boolean getBool(String string) {
        return getProperty(string, "false").trim().equalsIgnoreCase("true");
    }

    public double getDouble(String key, double defVal) {
        String val = getProperty(key);
        return val == null ? defVal : Double.parseDouble(val);
    }
    public double[] getDoubleArray(String key) {
        String str = getProperty(key);
        if (str == null) return null;
		StringTokenizer toki= new StringTokenizer(str);
        double[] ret= new double[toki.countTokens()];
        for (int i= 0; i < ret.length; i++)
            ret[i]= Double.parseDouble(toki.nextToken());
        return ret;
    }
    public int getInt(String key) {
        return Integer.parseInt(getProperty(key));
    }
    
    public int getInt(String key, int def){
    	String str = getProperty(key);
    	if (str==null) return def;
    	return Integer.parseInt(str);
    }
    
    public String getProperty(String key) {
        return super.getProperty(key);
    }
    public String[] getStringArray(String key, String delimiters) {
        String str = getProperty(key);
        if (str == null) return null;
        StringTokenizer toki= new StringTokenizer(str, delimiters);
        String[] ret= new String[toki.countTokens()];
        for (int i= 0; i < ret.length; i++)
            ret[i]= toki.nextToken();
        return ret;
    }
    public Matrix getTransformation(String key) {
        double[] matrix= getDoubleArray(key);
        if (matrix == null) return null;
        if (matrix.length != 16)
            throw new RuntimeException("wrong array length for transformation!");
        return new Matrix(matrix);
    }
    private void init(Input input, ConfigurationAttributes parentAttr) {
        try {
            InputStream in=input.getInputStream();
            try {
                load(in);
            } finally {
                in.close();
            }
            // load parent prop file
            super.defaults=(parentAttr!=null)?
              parentAttr: new ConfigurationAttributes();
            String parentPropFileName= getProperty("parent");
            if (parentPropFileName != null)
                super.defaults.load(input.getRelativeInput(parentPropFileName).getInputStream());
        } catch (FileNotFoundException e) {
            Logger.getLogger("de.jreality").log(Level.WARNING,
                "file {0} : {1} not found!", new Object[]{input, getProperty("parent")});
        } catch (IOException e) {
            Logger.getLogger("de.jreality").log(Level.WARNING, input.toString(), e);
        }
    }
}
