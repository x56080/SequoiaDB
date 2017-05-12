package com.sequoiadb.ant.sdbtask;

import org.apache.tools.ant.BuildException;
import org.apache.tools.ant.Task;

//import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;

public class SdbGetCS extends Task {
	private String uuid = null;
	private String csName = null;
	//private boolean failonerror = false;
	
	public void setSdbhandle(String value)
	{
		uuid = value;
	}
	
	public void setCsname(String value)
	{
		csName = value;
	}
	
	/*
	public void setFailonerror(String value)
	{
		failonerror = Boolean.parseBoolean(value);
	}
	*/
	
	public void execute() {
		Object obj = this.getProject().getReference(uuid);
		if (! (obj instanceof Sequoiadb))
		{
			throw new BuildException("The SdbUUID" + uuid + " cannot get Sequoiadb Object.");			
		}
		
		try
		{
			Sequoiadb sdb = (Sequoiadb) obj;
			boolean csExist = sdb.isCollectionSpaceExist(csName);
			if(csExist){
				sdb.getCollectionSpace(csName);
			}else{
				throw new BuildException("The cs:" + csName
						+ " is not exist.");
			}
		}
		catch(BaseException e)
		{
				throw new BuildException(e.toString());
			}
		}

}
