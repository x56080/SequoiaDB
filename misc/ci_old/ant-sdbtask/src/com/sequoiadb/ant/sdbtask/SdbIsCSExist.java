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
public class SdbIsCSExist extends Task {

	private String uuid = null;
	private String csName = null;
	private boolean failonexist = false;

	public void setSdbhandle(String value) {
		uuid = value;
	}

	public void setCsname(String value) {
		csName = value;
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
		try {
			Sequoiadb sdb = (Sequoiadb) obj;
			boolean bExist = sdb.isCollectionSpaceExist(csName);

			if (!bExist) {
				if (!failonexist) {
					// If cs is not exist, and set fail on not exist, then throw
					// exception
					throw new BuildException("The cs:" + csName
							+ " is not exist.");
				}
			} else {
				if (failonexist) {
					// If cs is exist, and set fail on exist, then throw
					// exception
					throw new BuildException("The cs:" + csName + " is exist.");
				}
			}

		} catch (BaseException e) {
			throw new BuildException(e.toString());
		} finally {
			// nothing
		}
	}

}
