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

   Source File Name = HostName.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.ant.datatype;

import java.util.ArrayList;
import java.util.List;

import org.apache.tools.ant.BuildException;

import com.sequoiadb.base.ReplicaGroup;
import com.sequoiadb.base.Sequoiadb;

public abstract class HostName extends NodeGroup{
	private List<String> hostName = new ArrayList<String>();
	private List<com.sequoiadb.ant.datatype.Node> nodes = new ArrayList<com.sequoiadb.ant.datatype.Node>();
	public void start(Sequoiadb sdb) throws BuildException {
	try {
		ReplicaGroup group = sdb.getReplicaGroup(getName());
		
		
		for (com.sequoiadb.ant.datatype.Node nodeInfo : getNodeList()) {
			hostName.add(group.getNode(nodeInfo.getHost()).toString());
		}

	} catch (Exception e) {

		e.printStackTrace();

		throw new BuildException(e.toString());
	}
}
}
