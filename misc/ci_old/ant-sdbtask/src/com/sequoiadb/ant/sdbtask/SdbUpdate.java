/**
 * 
 */
package com.sequoiadb.ant.sdbtask;

import java.net.InetAddress;

import org.apache.tools.ant.BuildException;
import org.apache.tools.ant.Task;

import com.sequoiadb.ant.datatype.JsonElement;
import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;

/**
 * @author qiushanggao
 *
 */
public class SdbUpdate extends Task {
	private String uuid = null;
	private String CSName = null;
	private String CLName = null;	
	private JsonElement modifier = null;
	private JsonElement record = null;
	
	public void setSdbhandle(String value)
	{
		uuid = value;
	}
	public void setCsname(String value)
	{
		CSName = value;
	}
	public void setClname(String value)
	{
		CLName = value;
	}
		
	public JsonElement createModifer()
	{
		if (modifier != null)
		{
			throw new BuildException("Error: cannt set more than one record.");
		}
		
		modifier = new JsonElement();
		return modifier;
	}
	
	public JsonElement createQuery() {
		if (record != null) {
			throw new BuildException("Error: cannt set more than one record.");
		}

		record = new JsonElement();
		return record;
	}

	
	
	public void execute() {
		Object obj = this.getProject().getReference(uuid);
		if (! (obj instanceof Sequoiadb))
		{
			throw new BuildException("The SdbUUID" + uuid + " cannot get Sequoiadb Object.");			
		}
		
		try
		{
			Sequoiadb sdb = (Sequoiadb) obj;
			CollectionSpace cs = sdb.getCollectionSpace(CSName);
			DBCollection cl= cs.getCollection(CLName);
			
			cl.update(record.toBSONObj(), modifier.toBSONObj(), null);
			
		}
		catch(BaseException e)
		{
			log(e.toString());
			throw new BuildException(e.toString());
		}
	}

}
