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


package de.jreality.soft;

import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.io.PrintWriter;

import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.Viewer;

/**
 * This is an experimental PS viewer for jReality.
 * It is still verry rudimentary and rather a 
 * proof of concept thatn a full featured PS writer.
 * @version 1.0
 * @author <a href="mailto:hoffmann@math.tu-berlin.de">Tim Hoffmann</a>
 *
 */
public class PSViewer extends AbstractViewer implements Viewer {

    private String fileName;

    /**
     * 
     */
    public PSViewer(String file) {
        super();
        fileName =file;
    }


    public void render(int width, int height) {
        File f=new File(fileName);
        PrintWriter w;
        try {
            w = new PrintWriter(new FileWriter(f));
            rasterizer =new PSRasterizer(w);
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
        super.render(width, height);
        w.close();
    }


    /* (non-Javadoc)
     * @see de.jreality.scene.Viewer#getMetric()
     */
    public int getMetric() {
        // TODO Auto-generated method stub
        return 0;
    }


    /* (non-Javadoc)
     * @see de.jreality.scene.Viewer#setMetric(int)
     */
    public void setMetric(int sig) {
        // TODO Auto-generated method stub
        
    }


	public SceneGraphComponent getAuxiliaryRoot() {
		// TODO Auto-generated method stub
		return null;
	}


	public void setAuxiliaryRoot(SceneGraphComponent ar) {
		// TODO Auto-generated method stub
		
	}    
    

}
