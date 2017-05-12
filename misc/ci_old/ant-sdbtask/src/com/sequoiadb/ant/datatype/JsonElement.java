/**
 * 
 */
package com.sequoiadb.ant.datatype;

import org.apache.tools.ant.BuildException;
import org.bson.BSONObject;
import org.bson.util.JSON;

/**
 * @author qiushanggao
 *
 */
public class JsonElement {

	private String text = null;
	
	
	public void addText(String value)
	{
		text = value;
	}
	
	public BSONObject toBSONObj()
	{
		BSONObject obj = null;
		if (text != null)
		{
			obj = (BSONObject) JSON.parse(text);
		}
		
		if (obj == null)
		{
			throw new BuildException("Warning: BSON Object == null,  the text = " + text);
		}
		return obj;
	}
	
	public String toString()
	{
		return text.trim();
	}
}
