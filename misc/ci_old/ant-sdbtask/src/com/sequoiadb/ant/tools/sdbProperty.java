package com.sequoiadb.ant.tools;

import org.apache.tools.ant.Task;

public class sdbProperty extends Task{
	private String proName ; 
	private String proPort ; 
	private String name;
	private String value;
	
	public void setProName( String value )
	{
		this.proName = value ;
	}
	public void setProPort( String value )
	{
		this.proPort = value ; 
	}
	
	public String getProName()
	{
		return this.proName ; 
	}
	public String getProPort()
	{
		return this.proPort ; 
	}
	public void setName( String value ){
		this.name = value;
	}
	public void setValue( String value ){
		this.value = value;
	}
	public void execute(){
		getProject().setUserProperty(this.name, this.value);
	}

}
