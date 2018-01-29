/* Copyright (c) 1997-2018
   Ewgenij Gawrilow, Michael Joswig (Technische Universitaet Berlin, Germany)
   http://www.polymake.org

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2, or (at your option) any
   later version: http://www.gnu.org/licenses/gpl.txt.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
--------------------------------------------------------------------------------
*/

package de.tuberlin.polymake.common;

import java.io.BufferedReader;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.channels.Pipe;
import java.util.HashMap;

import de.tuberlin.polymake.common.geometry.EmbeddedGeometries;
import de.tuberlin.polymake.common.io.GeometryParserIf;
import de.tuberlin.polymake.common.io.SimpleGeometryParser;

public abstract class PolymakeControl { 
        
    protected boolean alive = true;
    protected boolean answer = false;
    protected SimpleGeometryParser parser;
    protected GeometryParserIf geometryParser;

    protected BufferedReader clientReader;
    protected BufferedReader psReader;
    protected Pipe.SinkChannel pipeSink;

    protected HashMap<String, PolymakeFrame> frameMap = new HashMap<String, PolymakeFrame>(8);


    /**
     * Queue containing messages to be sent to the client. 
     */
    protected MsgQueue clientQueue = new MsgQueue();

    protected ByteBuffer bbuf = ByteBuffer.allocate(32);

    protected EmbeddedGeometries geometry;

    public PolymakeControl( 
                           BufferedReader psReader, 
                           BufferedReader clientReader, 
                           Pipe.SinkChannel sink, 
                           GeometryParserIf gp) throws IOException
    {
        this.clientReader = clientReader;
        this.psReader = psReader;
        pipeSink = sink;
        geometryParser = gp;
        initGeometry();
    }

    public abstract void update() throws IOException;

    public abstract PolymakeFrame createFrame(String title);

    public void removeFrame(String title) throws IOException {
        answer = false;
        PolymakeFrame f = frameMap.remove(title);
        if (f != null) {
            f.forget();
        }
        if (frameMap.size() == 0) {
            alive = false;
//          bbuf.flip();
//          pipeSink.write(bbuf);
        }
    }

    /**
     * Put a message into the queue to be processed by the SelectorThread. 
     * 
     * @param msg                       message to be processed
     * @param ch                        'C' for clientChannel, 'P' for polymakeChannel
     * @param ans                       does this msg expect an answer
     * @throws IOException
     */
    public void putMessage(String msg, char ch, boolean ans) throws IOException {
        synchronized(clientQueue) {
            answer = ans;
            boolean notifyClient = clientQueue.isEmpty();
            clientQueue.pushBack(msg);
            if (notifyClient) {
                bbuf.clear();
                bbuf.putChar(ch);
                bbuf.flip();
                pipeSink.write(bbuf);
            }
        }
    }

    public void popMessage() {
        synchronized(clientQueue) {
            if (!clientQueue.isEmpty()) 
                clientQueue.popFront();
        }
    }

    public synchronized boolean getAnswer() {
        return answer;
    }

    public boolean isAlive() {
        return alive;
    }

    public String getMessage() {
        synchronized(clientQueue) {
            return (String)clientQueue.front();
        }
    }

    public void initGeometry() throws IOException {
        geometry = geometryParser.parse(psReader);
    }

    public GeometryParserIf getGeometryParser() {
        return geometryParser;
    }
}

// Local Variables:
// indent-tabs-mode:nil
// End:
