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

/**
 * 
 * @version 1.0
 * @author timh
 *
 */
public class HatchImager extends Imager {
    private final int BLACK = 255<<24;
    private final int WHITE = (255<<24)+(255<<16)+(255<<8)+(255);
    private final int EDGE = 300000000;
    public HatchImager() {
        super();
    }
    public void process(int[] pixels, int[] zBuf, int w, int h) {
        for(int i = 1;i< w-2;i++)
            for(int j = 1; j<h-2;j++) {
                int pos = i + w*j;
                if((zBuf[pos]-zBuf[pos+1]) > EDGE) {
                    pixels[pos-1] = WHITE;
                    pixels[pos] = WHITE;
                    pixels[pos+1] = BLACK;
                    pixels[pos+2] = BLACK;
                } else
                if(-(zBuf[pos-1]-zBuf[pos+1]) > EDGE) {
                    pixels[pos-1] = BLACK;
                    pixels[pos] = BLACK;
                    pixels[pos+1] = WHITE;
                    pixels[pos+2] = WHITE;
                }
                if((zBuf[pos]-zBuf[pos+w]) > EDGE) {
                    pixels[pos-w] = WHITE;
                    pixels[pos] = WHITE;
                    pixels[pos+w] = BLACK;
                    pixels[pos+2*w] = BLACK;
                }else
                if(-(zBuf[pos]-zBuf[pos+w]) > EDGE) {
                    pixels[pos-w] = BLACK;
                    pixels[pos] = BLACK;
                    pixels[pos+w] = WHITE;
                    pixels[pos+2*w] = WHITE;
                }
                
                int value  = (((pixels[pos]>>16)&255) + ((pixels[pos]>>8)&255) + ((pixels[pos])&255))/3;

                if(value>80) {
                    pixels[pos] = WHITE;
                    continue;
                }
                
                int m = value >20? 4:(value>5? 2:1);
                int iu = ((int) ((i+j)/4))%m;
                //int iu = ((int) ((v/scaleV)+(u/scaleU)))%m;
                if(iu ==0 ) {
                    pixels[pos] = BLACK;
                } else {
                    pixels[pos] = WHITE;
                }

                
            }

    }

}
