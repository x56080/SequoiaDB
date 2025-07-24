/*******************************************************************************

   Copyright (C) 2011-Present SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   
*******************************************************************************/
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
