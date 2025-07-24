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
package com.sequoiadb.ant.datatype;

import org.apache.tools.ant.BuildException;


import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.base.ReplicaGroup;
import com.sequoiadb.exception.BaseException;

/**
 * @author qiushanggao
 * 
 */
public class DataNodeGroup extends NodeGroup {

	@Override
	public void start(Sequoiadb sdb) throws BuildException {
		try {
			ReplicaGroup group = sdb.getReplicaGroup(getName());

			if (group == null) {
				
				group = sdb.createReplicaGroup(getName());
			}

			for (com.sequoiadb.ant.datatype.Node nodeInfo : getNodeList()) {
				com.sequoiadb.base.Node node = group.getNode(nodeInfo.getHost(),
						nodeInfo.getBasePort());

				if (node == null) {
					System.out.println("host :"+nodeInfo.getHost()+"\n" +
							"port : " +nodeInfo.getBasePort() +"\n" +
							"path : " + nodeInfo.getDbpath() + "\n" +
							"configMap :"+nodeInfo.getConfigMap().toString() + "\n\n\n"
							);
					
					group.createNode(nodeInfo.getHost(),
							nodeInfo.getBasePort(), nodeInfo.getDbpath(),
							nodeInfo.getConfigMap());
				} else {
					throw new BuildException("Node repeat: hostname="
							+ nodeInfo.getHost() + "servicename:"
							+ nodeInfo.getBasePort());
				}
			}

			group.start();

		} catch (Exception e) {

			e.printStackTrace();

			throw new BuildException(e.toString());
		}
	}

	public void waitForStart(Sequoiadb sdb, long timeout) throws BuildException {

		ReplicaGroup group = sdb.getReplicaGroup(getName());

		// Wait for group select master, max wait time is 600sec;
		int i = 0;
		while (true) {
			try {
				com.sequoiadb.base.Node masterNode = group.getMaster();
				if (masterNode != null) {
					break;
				}
			} catch (BaseException baseException) {
			} 
			
			i++;
			if (i >= timeout) {
				throw new BuildException("Group:" + this.getName()
						+ " select master timeout.");
			}
			
			try
			{
				Thread.sleep(1000);
			}
			catch (InterruptedException e) {
			}
		}
		System.out.println("The Group : " + this.getName() + " select master time is " + i + " seconds.");
		

	}

}
