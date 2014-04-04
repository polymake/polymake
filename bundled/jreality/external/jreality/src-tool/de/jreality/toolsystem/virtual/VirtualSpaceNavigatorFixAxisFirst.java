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


package de.jreality.toolsystem.virtual;

import java.util.LinkedList;
import java.util.List;
import java.util.Map;

import de.jreality.scene.tool.AxisState;
import de.jreality.scene.tool.InputSlot;
import de.jreality.toolsystem.MissingSlotException;
import de.jreality.toolsystem.ToolEvent;
import de.jreality.toolsystem.VirtualDevice;
import de.jreality.toolsystem.VirtualDeviceContext;

/**
 * 
 * Transforms an Axis.
 *
 */
public class VirtualSpaceNavigatorFixAxisFirst implements VirtualDevice {

	InputSlot inSlot, outSlot;
	
	AxisState state;

	double eps=0.06;

	
	/**
	 * Take the average if the next Value is to far away from the average.
	 * After assumed Failure:
	 * if the next Value is not near(failureToleranceRatio) the assumed Failure
	 * the assumed Failure will not be respected by the following averages    
	 *  
	 */
	
	static final int num=4;
	List<Double> list= new LinkedList<Double>();
	double CriticalRatio=0.75;// (<1)!!
	double failureToleranceRatio=0.4;//(<.5)!!
	boolean assumeFailure=false;
	
	private double calcAverage(){
		double average=0;
		for(double d: list){average+=d;}
		if(list.size()>0)
			average/=list.size();
		return average;
	}
	private double calcAverage2(){
		double average=0;
		double steepnes=0;
		double first=list.get(0);
		
		for (int i = 1; i < list.size(); i++) {
			steepnes+=(list.get(i)-first)/i;
		}
		steepnes/=list.size()-1;
		
		for(double d: list){average+=d;}
		if(list.size()>0)
			average/=list.size();
		return average+steepnes/2;
	}
	
	public double goodVal(double newOne){
		//Fehlerart:
		// vereinzelt ganze vielfache der Werte 
		// und vereinzelt 0(Ausgleichend in der Naehe)
		boolean remove=false;		
		if(assumeFailure){//take of of average?
			double last=list.get(list.size()-1);//Vorgaenger
			//Abweichung zu stark:
			if(Math.abs(newOne-last)>failureToleranceRatio*last)remove=true;
			// Abweichung nach oben immer ok
			if((newOne-last)*Math.signum(last)>0)remove=false;
			if(remove)list.remove(list.size()-1);
		} 
		// average:
		double average=calcAverage();
//		if(list.size()>2)
//			average=calcAverage2();
		// tolerance:
		double tolerance=Math.abs(CriticalRatio*average);
		// reset content:
		list.add(newOne);
		if(list.size()==num+1) list.remove(0);
		// return:
		double result=average;
		assumeFailure=true;
		if(Math.abs(Math.abs(average)-Math.abs(newOne))<tolerance){
			assumeFailure=false;
			result=newOne;
		}
		if(newOne==0){
			assumeFailure=true;
			result=average;
		}
		if(assumeFailure&!remove){// der neue Wert ist doch glaubwuerdig
			assumeFailure=false;
			result=newOne;
		}
		
//		System.out.println("VirtualSpaceNavigatorFixAxis2.goodVal(new|res)"+newOne+1+"|"+result+1000);
		return result;
	}
	
	
	public ToolEvent process(VirtualDeviceContext context) throws MissingSlotException {
		double val = context.getAxisState(inSlot).doubleValue();
		val=goodVal(val);
		return new ToolEvent(context.getEvent().getSource(), context.getEvent().getTimeStamp(), outSlot, new AxisState(val)); 
	}

	public void initialize(List inputSlots, InputSlot result,
			Map configuration) {
		inSlot = (InputSlot) inputSlots.get(0);
		outSlot = result;
	}

	public void dispose() {
	}

	public String getName() {
		return "BumpAxis";
	}

}
