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

public class Triangle extends AbstractPolygon {
    private final double[] p0 = new double[VERTEX_LENGTH];

    private final double[] p1 = new double[VERTEX_LENGTH];

    private final double[] p2 = new double[VERTEX_LENGTH];

    public Triangle() {
        super();
        // TODO Auto-generated constructor stub
    }

    @Override
    public final double[] getPoint(int i) {
        switch (i) {
        case 0:
            return p0;
        case 1:
            return p1;
        case 2:
            return p2;
        default:
            throw new IllegalArgumentException(
                    "a triangle has only three points");
        }
    }

    public final double[] getP1() {
        return p1;
    }

    public final double[] getP2() {
        return p2;
    }

    public final double[] getP0() {
        return p0;
    }

    public final double getP1(final int i) {
        return p1[i];
    }

    public final double getP2(final int i) {
        return p2[i];
    }

    public final double getP0(final int i) {
        return p0[i];
    }

    public final void setP1(final int i, final double v) {
        p1[i] = v;
    }

    public final void setP2(final int i, final double v) {
        p2[i] = v;
    }

    public final void setP0(final int i, final double v) {
        p0[i] = v;
    }

    public final double getCenterZ() {
        return (p0[SZ]/p0[SW] + p1[SZ]/p1[SW] + p2[SZ]/p2[SW]) / 3;
    }

    @Override
    public int getLength() {
        return 3;
    }
    
    public String toString() {
        StringBuilder b = new StringBuilder("t[\n");
        b.append(p0[0]);
        b.append(", ");
        b.append(p0[1]);
        b.append(", ");
        b.append(p0[2]);
        b.append(",\n");

    
        b.append(p1[0]);
        b.append(", ");
        b.append(p1[1]);
        b.append(", ");
        b.append(p1[2]);
        b.append(",\n");

    
        b.append(p2[0]);
        b.append(", ");
        b.append(p2[1]);
        b.append(", ");
        b.append(p2[2]);
        b.append("\n]");

        return b.toString();
    }

}
