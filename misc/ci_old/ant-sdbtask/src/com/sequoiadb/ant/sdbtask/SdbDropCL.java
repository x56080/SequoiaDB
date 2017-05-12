package com.sequoiadb.ant.sdbtask;

import org.apache.tools.ant.BuildException;
import org.apache.tools.ant.Task;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;

public class SdbDropCL extends Task {

	private String uuid = null;
	private String csName = null;
	private String clName = null;
	private boolean failonerror = true;
	
	public void setSdbhandle(String value)
	{
		uuid = value;
	}
	
	public void setCsname(String value)
	{
		csName = value;
	}
	
	public void setClname(String value)
	{
		clName = value;
	}
	
	public void setFailonerror(String value)
	{
		failonerror = Boolean.parseBoolean(value);
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
			CollectionSpace space = sdb.getCollectionSpace(csName);
			space.dropCollection(clName);
		}
		catch(BaseException e)
		{
			if (failonerror)
			{
				throw new BuildException(e.toString());
			}
			else
			{
				log("Failed to dropcs(" + csName + ") , but not throw exception. exception=" + e);
			}
		}
	}

}
