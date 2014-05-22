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


package de.jreality.io.jrs;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.util.zip.DataFormatException;
import java.util.zip.Deflater;
import java.util.zip.Inflater;

import com.thoughtworks.xstream.converters.Converter;
import com.thoughtworks.xstream.converters.MarshallingContext;
import com.thoughtworks.xstream.converters.UnmarshallingContext;
import com.thoughtworks.xstream.io.HierarchicalStreamReader;
import com.thoughtworks.xstream.io.HierarchicalStreamWriter;
import com.thoughtworks.xstream.mapper.Mapper;

import de.jreality.shader.ImageData;

/**
 * 
 * @author weissman
 *
 */
class ImageDataConverter implements Converter {

  Mapper mapper;

  public ImageDataConverter(Mapper mapper) {
    this.mapper = mapper;
  }

  public boolean canConvert(Class type) {
    return type == ImageData.class;
  }

  public void marshal(Object source, HierarchicalStreamWriter writer, MarshallingContext context) {
    ImageData id = (ImageData)source;
	byte[] data = id.getByteArray();
    writer.addAttribute("width", ""+id.getWidth());
    writer.addAttribute("height", ""+id.getHeight());
    context.convertAnother(compress(data));
  }

  public Object unmarshal(HierarchicalStreamReader reader, UnmarshallingContext context) {
	  int w=-1, h=-1;
	  byte[] data=null;
	  if (reader.getAttributeCount() > 0) {
		  w=Integer.parseInt(reader.getAttribute("width"));
		  h=Integer.parseInt(reader.getAttribute("height"));
		  data = (byte[]) context.convertAnother(null, byte[].class);
		  data = uncompress(data);
		  
	  } else {
		  // read old format (from default XStream serialization)
		  reader.moveDown();
		  data = (byte[]) context.convertAnother(null, byte[].class);
		  reader.moveUp();
		  reader.moveDown();
		  w = (Integer) context.convertAnother(null, int.class);
		  reader.moveUp();
		  reader.moveDown();
		  h = (Integer) context.convertAnother(null, int.class);
		  reader.moveUp();
	  }
	  return new ImageData(data, w, h);
  }

  private static byte[] uncompress(byte[] data) {
	  Inflater decompressor = new Inflater();
	  decompressor.setInput(data);
	  ByteArrayOutputStream bos = new ByteArrayOutputStream(data.length);
	  byte[] buf = new byte[1024*1024];
	  while (!decompressor.finished()) {
		  try {
			  int count = decompressor.inflate(buf);
			  bos.write(buf, 0, count);
		  } catch (DataFormatException e) {
		  }
	  }
	  try {
		  bos.close();
	  } catch (IOException e) {
	  }
	  return bos.toByteArray();
  }
  
  private static byte[] compress(byte[] data) {
	  //System.out.println("data.length="+data.length);
	  Deflater compressor = new Deflater();
	  compressor.setLevel(Deflater.BEST_COMPRESSION);
	  compressor.setInput(data);
	  compressor.finish();
	  ByteArrayOutputStream bos = new ByteArrayOutputStream(data.length/2);
	  byte[] buf = new byte[1024*1024];
	  while (!compressor.finished()) {
		  int count = compressor.deflate(buf);
		  bos.write(buf, 0, count);
	  }
	  try {
		  bos.close();
	  } catch (IOException e) {
	  }
	  byte[] compressedData = bos.toByteArray();
	  //System.out.println("compressedData.length="+compressedData.length+" compression="+((double)compressedData.length/data.length));
	return compressedData;
  }

}