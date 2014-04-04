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

import java.beans.Expression;
import java.io.IOException;
import java.io.InputStream;
import java.lang.reflect.Constructor;
import java.lang.reflect.InvocationTargetException;

import de.jreality.scene.SceneGraphComponent;
import de.jreality.util.Input;
import de.jreality.util.LoggingSystem;
/**
 * A reader for VRML1.0 and VRML2.0, based on an ANTRL parser in package de.jreality.reader.vrml}.
 * @author gunn, gonska
 *
 */
public class ReaderVRML extends AbstractReader {

	public ReaderVRML() {
		super();
	}

	public void setInput(Input input) throws IOException {
		try {
      Constructor lexC = Class.forName("de.jreality.reader.vrml.VRMLV1Lexer").getConstructor(new Class[]{InputStream.class});
 			Object lexer = lexC.newInstance(new Object[]{input.getInputStream()});
      
      Constructor parseC = Class.forName("de.jreality.reader.vrml.VRMLV1Parser").getConstructor(new Class[]{Class.forName("antlr.TokenStream")});
      Object parser = parseC.newInstance(new Object[]{lexer});
			
      Expression parse = new Expression(parser, "vrmlFile", null);
      root = (SceneGraphComponent) parse.getValue();
    } catch (ClassNotFoundException e) {
      LoggingSystem.getLogger(this).severe("VRML 1 parsing failed, call ANTLR first!");
      e.printStackTrace();
    } catch (NoSuchMethodException e) {
      throw new Error(e);
//    } catch (IllegalArgumentException e) {
//      throw new Error();
    } catch (InstantiationException e) {
      throw new Error(e);
    } catch (IllegalAccessException e) {
      throw new Error(e);
    } catch (InvocationTargetException e) {
      throw new Error(e);
    } catch (Exception e) {
      LoggingSystem.getLogger(this).severe("parsing "+input+" failed: "+e.getMessage());
    }
    if (root == null) throw new IOException("could not load input: "+input); 
	}
}
