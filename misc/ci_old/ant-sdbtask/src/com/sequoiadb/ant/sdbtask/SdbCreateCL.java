package com.sequoiadb.ant.sdbtask;

import org.apache.tools.ant.BuildException;
import org.apache.tools.ant.Task;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.Sequoiadb;
import org.bson.*;

import com.sequoiadb.exception.BaseException;


public class SdbCreateCL extends Task {
	private String uuid = null;
	private String csName = null;
	private String clName = null;
	private boolean failonerror = false;
	private String replSize = null ;
	
	public void setReplSize( String value )
	{
		this.replSize = value ;
	}
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
			BSONObject bson = new BasicBSONObject();
			//bson = null ;
			if( this.replSize != null )
				bson.put("ReplSize", Integer.parseInt( this.replSize ) );
			else
				bson = null ;

			space.createCollection( clName , bson ) ; 
		}
		catch(BaseException e)
		{
			if (failonerror)
			{
				throw new BuildException(e.toString());
			}
			else
			{
				log("Failed to createcl(" + clName + ") , but not throw exception. exception=" + e);
			}
		}
	}

}
