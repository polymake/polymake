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


package de.jreality.tools;

import de.jreality.math.Pn;

/**
 * @author bleicher
 *
 */

public class ShowPropertiesToolLogger{
	private String log;
	private boolean homogeneous=true;
	
	
	public ShowPropertiesToolLogger(){
	}
	
	public ShowPropertiesToolLogger(String str){
		this();
		setLine(str);
	}
	
	public void addLine(String newLine){
		if(log==null) log=new String(newLine);
		else log=log+"\n"+newLine;
	}
	
	public void addLine(String description, double[] point){
		addLine(description+": ");
		addPoint(point);
	}
	
	public void addLine(String description,int[] ints){
		addLine(description+": ");
		addPoint(ints);
	}
	
	public void addLine(double[] point){
		addLine("point",point);
	}
	
	public void addPoint(double[] point){
		if(point==null){
			log=log+"null";
			return;
		}
		if(point.length<1) return;
		
		String newStr="";		
		int length=point.length;
		double[] writePoint=point;
		if(length==4){
			if(!homogeneous){		
				length-=1;
				writePoint=new double[length];			
				Pn.dehomogenize(writePoint,point);
				if(point[3]==0) newStr=newStr+" (inf) ";
			}
		}
		
		newStr=newStr+point[0];
		for(int i=1;i<length;i++){
			newStr=newStr+", "+writePoint[i];
		}
		log=log+newStr;
	}
	
	public void addPoint(int[] ints){
		if(ints==null){
			log=log+"null";
			return;
		}
		String newStr=""+ints[0];		
		for(int i=1;i<ints.length;i++){
			newStr=newStr+", "+ints[i];
		}
		log=log+newStr;
	}	
	
	public void clear(){
		log=null;
	}
	
	public void setLine(String line){
		clear();
		addLine(line);
	}
	
	public String getLog(){
		if(log==null) log="";
		return log;
	}
	
	public void setHomogeneousLogging(boolean hom){
		homogeneous=hom;
	}
}
