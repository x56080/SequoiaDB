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

   Source File Name = SdbCreateIndex.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.ant.sdbtask;

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
public class SdbCreateIndex extends Task {

	private String uuid = null;
	private String indexName = null;
	private JsonElement key = null;
	private String csName = null;
	private String clName = null;
	private boolean isUnique = false;
	private boolean enforced = false;

	private boolean failonerror = true;
	
	
	public void setSdbhandle(String value)
	{
		uuid = value;
	}
	
	public void setName(String value)
	{
		indexName = value;
	}
	
	public JsonElement createKey()
	{
		if (key != null)
		{
			throw new BuildException("Error: cannt set more than one record.");
		}
		
		key = new JsonElement();
		return key;
	}
	public void setCsname(String value)
	{
		csName = value;
	}
	public void setClname(String value)
	{
		clName = value;
	}
	
	public void setIsunique(String value)
	{
		this.isUnique = Boolean.getBoolean(value);
	}
	
	public void setEnforced(String value)
	{
		this.enforced = Boolean.getBoolean(value);
	}
	
	public void setFailonerror(String value)
	{
		failonerror = Boolean.parseBoolean(value);
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
			CollectionSpace space = sdb.getCollectionSpace(csName);
			DBCollection cl = space.getCollection(clName);
			
			log("CreateIndex(" + indexName + "," + key.toString() + ")");
			cl.createIndex(indexName, key.toBSONObj(), isUnique, enforced);
		}
		catch(BaseException e)
		{
			if (failonerror)
			{
				throw new BuildException(e);
			}
			else
			{
				log("Failed to create index , but not throw exception. exception=" + e);
			}
		}
		
	}
}
