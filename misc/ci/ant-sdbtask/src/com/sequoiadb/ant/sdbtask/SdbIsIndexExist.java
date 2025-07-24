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

   Source File Name = SdbIsIndexExist.java

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

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;

public class SdbIsIndexExist extends Task {
	private String uuid = null;

	private String clName = null;
	private String csName = null;
	private String indexName = null;

	private boolean failonexist = false;

	public void setSdbhandle(String value) {
		uuid = value;
	}

	public void setClname(String value) {
		clName = value;
	}

	public void setCsname(String value) {
		csName = value;
	}

	public void setIndexname(String value) {
		indexName = value;
	}

	public void setFailonexist(String value) {
		failonexist = Boolean.parseBoolean(value);
	}

	public void execute() {
		Object obj = this.getProject().getReference(uuid);
		if (!(obj instanceof Sequoiadb)) {
			throw new BuildException("The SdbUUID" + uuid
					+ " cannot get Sequoiadb Object.");
		}

		log("faileonexist=" + failonexist);
		DBCursor cursor = null;
		try {
			Sequoiadb sdb = (Sequoiadb) obj;
			CollectionSpace space = sdb.getCollectionSpace(csName);
			DBCollection cl = space.getCollection(clName);

			
			if (indexName != null) {
				cursor = cl.getIndex(indexName);
			} else {
				cursor = cl.getIndexes();
			}

			if (cursor != null && cursor.hasNext()) {
				if (failonexist)
				{
					throw new BuildException("Find index:"
							+ indexName + " in " + csName + "." + clName);
				}

			} else {
				if (!failonexist) {
					
					log("Throw exception: faileonexist=" + failonexist);
					
					throw new BuildException("Failed to find index:"
							+ indexName + " in " + csName + "." + clName);
				}
				
				log("Can't find index, but not throw exception, because of faileonexist=" + failonexist);
			}
		} catch (BaseException e) {
			throw new BuildException(e.toString());
		} finally {
			if (cursor != null)
			{
				cursor.close();
			}
		}
	}

}
