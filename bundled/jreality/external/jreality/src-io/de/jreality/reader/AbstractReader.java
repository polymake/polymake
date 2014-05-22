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

import java.io.File;
import java.io.IOException;
import java.net.URL;

import de.jreality.scene.SceneGraphComponent;
import de.jreality.util.Input;

/**
 * Abstract class for a SceneReader. Implement setInput so that
 * the Input is processed and the component assigned to root.
 */
public abstract class AbstractReader implements SceneReader
{
  protected SceneGraphComponent root;
  protected Input input;

  public void setInput(Input input) throws IOException
  {
    this.input=input;
  }

  public SceneGraphComponent getComponent()
  {
    return root;
  }

  /**
   * convenience method for reading from an Input.
   * 
   * @param input the input
   * @return the root component of the scenegraph created from the input
   * @throws IOException
   */
  public SceneGraphComponent read(Input input) throws IOException
  {
    setInput(input);
    return getComponent();
  }
  
  /**
   * convenience method for reading from a URL.
   * 
   * @param input the input URL
   * @return the root component of the scenegraph created from the input
   * @throws IOException
   */
  public SceneGraphComponent read(URL input) throws IOException
  {
    setInput(new Input(input));
    return getComponent();
  }
  
  /**
   * convenience method for reading a File.
   * 
   * @param input the input file
   * @return the root component of the scenegraph created from the input
   * @throws IOException
   */
  public SceneGraphComponent read(File input) throws IOException
  {
    setInput(new Input(input));
    return getComponent();
  }
}
