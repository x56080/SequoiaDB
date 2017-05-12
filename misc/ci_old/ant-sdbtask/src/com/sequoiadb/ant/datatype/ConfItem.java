/**
 * 
 */
package com.sequoiadb.ant.datatype;

/**
 * @author qiushanggao
 *
 */
public class ConfItem {
	private String name;
	private String value;
	
	public void setName(String value)
	{
		name = value;
	}
	
	public String getName()
	{
		return name;
	}
	
	public void setValue(String value)
	{
		this.value = value;
	}
	
	public String getValue()
	{
		return value;
	}
	
	public String toString()
	{
		String msg = "ConfItem{";

		msg += "name:" + name +",";
		msg += "value:" + value;
		
		msg += "}";
		
		return msg;
	}

}
