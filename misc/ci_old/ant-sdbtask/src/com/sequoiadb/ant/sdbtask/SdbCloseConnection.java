/**
 * 
 */
package com.sequoiadb.ant.sdbtask;

import org.apache.tools.ant.BuildException;
import org.apache.tools.ant.Task;

import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;

/**
 * @author qiushanggao
 * 
 */
public class SdbCloseConnection extends Task {
	
	private String uuid = null;
	
	public void setSdbhandle(String value)
	{
		uuid = value;
	}
	
	public void execute() {
		Object obj = this.getProject().getReference(uuid);
		
		
		if ( !(obj instanceof Sequoiadb))
		{
			log("Cann't find Sequoiadb obj by uuid(" + uuid +")");
			
			throw new BuildException("Cann't find Sequoiadb obj by uuid(" + uuid +")");
		}
		
		try
		{
			Sequoiadb sdb = (Sequoiadb) obj;
			sdb.disconnect();
			
			this.getProject().getReferences().remove(uuid);
		}
		catch(BaseException e)
		{
			throw new BuildException(e);
		}
		
	}
}
