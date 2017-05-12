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
