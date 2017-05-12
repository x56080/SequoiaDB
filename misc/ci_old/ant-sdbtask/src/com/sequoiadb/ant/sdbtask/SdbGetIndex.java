/**
 * 
 */
package com.sequoiadb.ant.sdbtask;

import java.util.UUID;

import org.apache.tools.ant.BuildException;
import org.apache.tools.ant.Task;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;

/**
 * @author qiushanggao
 *
 */
public class SdbGetIndex extends Task {
	private String uuid = null;
	private boolean failonerror = true;
	
	private String clName = null;
	private String csName = null;
	private String indexName = null;
	private String cursorHandle = null;
	
	public void setFailonerror(String value)
	{
		failonerror = Boolean.getBoolean(value);
	}
	public void setSdbhandle(String value)
	{
		uuid = value;
	}
	public void setClname(String value)
	{
		clName = value;
	}
	public void setCsname(String value)
	{
		csName = value;
	}
	public void setIndexname(String value)
	{
		indexName = value;
	}
	public void setCusorproperty(String value)
	{
		cursorHandle = value;
	}
	
	public void execute()
	{
		Object obj = this.getProject().getReference(uuid);
		if (! (obj instanceof Sequoiadb))
		{
			throw new BuildException("The SdbUUID" + uuid + " cannot get Sequoiadb Object.");			
		}
		
		try
		{
			Sequoiadb sdb = (Sequoiadb) obj;
			CollectionSpace space = sdb.getCollectionSpace(csName);
			DBCollection cl = space.getCollection(clName);
			
			DBCursor cusor = null;
			if (indexName != null)
			{
				cusor = cl.getIndex(indexName);
			}
			else
			{
				cusor = cl.getIndexes();
			}
			
			UUID uuid = UUID.randomUUID();
			String strUUID = uuid.toString();

			this.getProject().addReference(strUUID, cusor);
			this.getProject().setProperty(this.cursorHandle, strUUID);
			
		}
		catch(BaseException e)
		{
			if (failonerror)
			{
				throw new BuildException(e.toString());
			}
			else
			{
				log("Failed to get index(" + indexName + "). exception=" + e);
			}
		}
	}
}
