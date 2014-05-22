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


import java.io.PrintWriter;
import java.io.StringWriter;
import java.util.logging.ConsoleHandler;
import java.util.logging.Formatter;
import java.util.logging.Handler;
import java.util.logging.Level;
import java.util.logging.LogRecord;
import java.util.logging.Logger;
import java.util.logging.SimpleFormatter;


/**
 * LoggingSystem for jReality. This sets up a base logger
 * called de.jreality that has default Level=Level.SEVERE
 * and does not pass records to the parent logger.
 * 
 * For debugging special packages simply set the Level for
 * the "package"-Logger to what you want.
 *
 * @author weissman
 */
public class LoggingSystem {
 
    private static final Level GLOBAL_LEVEL = Level.SEVERE;
    
    //Singleton object for the logging sytem.
    private static LoggingSystem logSystem = new LoggingSystem();

    //the logger.
    private final Logger logger;
    
    private final Formatter formatter = new SimpleFormatter() {

        public synchronized String format(LogRecord record) {
            StringBuffer sb = new StringBuffer();
//            if (record.getSourceClassName() != null) {
//                sb.append(record.getSourceClassName());
//            } else {
//                sb.append(record.getLoggerName());
//            }
            String message = formatMessage(record);
            sb.append(record.getLevel().getLocalizedName());
            if (record.getSourceMethodName() != null) {
                sb.append(" [");
                sb.append(record.getSourceClassName().substring(record.getSourceClassName().lastIndexOf('.')+1)).append(".");
                sb.append(record.getSourceMethodName());
            }
            sb.append("]: ");
            sb.append(message);
            sb.append("\n");
            if (record.getThrown() != null) {
                try {
                    StringWriter sw = new StringWriter();
                    PrintWriter pw = new PrintWriter(sw);
                    record.getThrown().printStackTrace(pw);
                    pw.close();
                    sb.append(sw.toString());
                } catch (Exception ex) {
                }
            }
            return sb.toString();
        }
    };

    private LoggingSystem() {
        logger = Logger.getLogger("de.jreality");
        try {
            setDebugUse();
        } catch (SecurityException se) {
            logger.info("no permission to change log level");
        }
    }

    private void setDebugUse() {
        // avoid logging on parent:
        logger.setUseParentHandlers(false);
        Handler handler=new ConsoleHandler();
        handler.setFormatter(formatter);
        logger.addHandler(handler);
        logger.setLevel(GLOBAL_LEVEL);
        handler.setLevel(Level.ALL);
        // make debugging loggers noisy
        Logger.getLogger("de.jreality.scene.geometry").setLevel(Level.FINER);
//        Logger.getLogger("de.jreality.toolsystem").setLevel(Level.FINER);
//        Logger.getLogger("de.jreality.scene.pick").setLevel(Level.FINE);
//        Logger.getLogger("de.jreality.math").setLevel(Level.FINER);
//        Logger.getLogger("de.jreality.io").setLevel(Level.INFO);
        Logger.getLogger("de.jreality.reader").setLevel(Level.FINEST);
//        Logger.getLogger("de.jreality.backends").setLevel(Level.FINE);
        Logger.getLogger("de.jreality.renderman").setLevel(Level.FINE);
    }

//    public void setLogToFile(String fileName) throws IOException {
//      logger.removeHandler(handler);
//      handler = new FileHandler(fileName);
//      handler.setFormatter(new SimpleFormatter());
//      logger.addHandler(handler);
//    }
    
    /**
     * factory method to get a logger. Usually this is the right
     * method to get a logger - LoggingSystem.getLogger(this).log(...)
     * @param
     * @return a logger named like the package of the given object
     */
    public static Logger getLogger(Object o) {
      try {
        return logSystem.getLog(o.getClass());
      } catch (Throwable t) {
        return Logger.getLogger("de.jreality");
      }
    }
    
    /**
     * factory method to get a logger. This is the right
     * method to get a logger in a static context - LoggingSystem.getLogger(MyCurrentClass.class).log(...)
     * @param
     * @return a logger named like the package of the given class
     */
    public static Logger getLogger(Class clazz) {
      try {
        return logSystem.getLog(clazz);
      } catch (Throwable t) {
        return Logger.getLogger("de.jreality");
      }
    }
    
    /**
     *
     * @return returns a logger for the package of the given class
     */
    private Logger getLog(Class clazz) {
        Package p = clazz.getPackage();
        if (p == null) return logger;
        String name = p.getName();
        return Logger.getLogger(name);
    }
}
