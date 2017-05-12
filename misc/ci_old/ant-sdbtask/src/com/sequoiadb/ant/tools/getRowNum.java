package com.sequoiadb.ant.tools;

import org.apache.tools.ant.Task;

public class getRowNum  extends Task{
	private String propertyName;
	
	public void setPropertyName( String value)
	{
		this.propertyName = value;
	}
	public void execute(){
		String lineNum = Integer.toString(this.getLocation()
				.getLineNumber() );
		System.out.println("line number is "+lineNum);
		this.getProject().setProperty(this.propertyName, lineNum );
	}

}
