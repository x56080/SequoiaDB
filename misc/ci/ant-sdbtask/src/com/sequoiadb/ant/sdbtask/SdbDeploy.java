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

   Source File Name = SdbDeploy.java

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

import java.util.ArrayList;
import java.util.List;

import org.apache.tools.ant.Task;

import com.sequoiadb.ant.datatype.*;
import com.sequoiadb.base.Sequoiadb;

public class SdbDeploy extends Task {
	private String hostName;
	private String coordport;

	private List<NodeGroup> NodeGroups = new ArrayList<NodeGroup>();

	// private List<CatalogNode> catalogNodes = new ArrayList<CatalogNodes>();

	public void setHost(String value) {
		hostName = value;
	}

	public void setCoordport(String value) {
		coordport = value;
	}

	public NodeGroup createCatagroup() {
		NodeGroup group = new CataNodeGroup();
		NodeGroups.add(group);
		return group;
	}

	public NodeGroup createDatagroup() {
		NodeGroup group = new DataNodeGroup();
		NodeGroups.add(group);
		return group;
	}

	public void execute() {
		
		
		Sequoiadb sdb = new Sequoiadb(this.hostName,
				Integer.parseInt(this.coordport), "", "");

		for (NodeGroup group : NodeGroups) {
			group.start(sdb);
		}

		for (NodeGroup group : NodeGroups) {
			group.waitForStart(sdb, 600);
		}

	}
}
