package com.sequoiadb.ant.sdbtask;


import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Date;



import org.apache.tools.ant.Task;

public class SdbTime extends Task{
	String beginTime ; 
	String endTime ; 
	String output;
	String value;

	public void setBeginTime( String value ){
		this.beginTime=value ;
	}	
	public void setEndTime(String value){
		this.endTime=value;
	}
	public void setOutput(String value){
		this.output=value;
	}
	public void setValue(String value){
		this.value = value;
	}
	
	public long getDurationTestcase(){
		Date timeBegin;
		Date timeEnd;	
		long diff = 0;

		try{
			DateFormat format1 = new SimpleDateFormat("yyyy-MM-dd-hh.mm.ss");
			timeBegin=format1.parse(beginTime);	
			DateFormat format2 = new SimpleDateFormat("yyyy-MM-dd-hh.mm.ss");
			timeEnd=format2.parse(endTime);
			
			diff=timeEnd.getTime()-timeBegin.getTime();
			diff=diff/1000;
		}catch(Exception e){
			e.printStackTrace();
		}
		return diff;
	}
	
	public void execute() {
		long screen = getDurationTestcase();
		value=Long.toString(screen);
		getProject().setUserProperty(this.output, this.value);
	}
}