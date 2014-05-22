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


package de.jreality.softviewer;


import java.awt.Dimension;
import java.awt.image.BufferedImage;


/**
 * 
 * @version 1.0
 * @author <a href="mailto:hoffmann@math.tu-berlin.de">Tim Hoffmann</a>
 *
 */
public class Renderer  extends AbstractRenderer {
    final BufferedImage img;
    protected final Dimension d;
    protected final int[] pixels;
    
    
    public Renderer(BufferedImage bi ) {
        this(bi,new int[bi.getWidth()* bi.getHeight()]);
    }
  private Renderer(BufferedImage bi,int[] pixels ) {
    super(new DoubleTriangleRasterizer(pixels), false, false);
    //pixels = new int[bi.getWidth()* bi.getHeight()];
    this.pixels = pixels;
    img = bi;
    d = new Dimension(bi.getWidth(), bi.getHeight());
    
    } 
  
   
	public void render() {
          render(d.width, d.height);
      }
      public void update() {
          img.getRaster().setDataElements(0, 0, d.width, d.height, pixels);
          //img.setRGB(0, 0, d.width, d.height, pixels,0,d.width);
          //Image img  = Toolkit.getDefaultToolkit().createImage(new MemoryImageSource(d.width,d.height,pixels,0,d.width));
          //System.out.println(" image type "+(img instanceof VolatileImage));
      }


}

