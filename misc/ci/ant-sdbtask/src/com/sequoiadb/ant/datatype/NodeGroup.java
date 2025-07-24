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

import java.util.ArrayList;
import java.util.List;
import org.apache.tools.ant.BuildException;
import com.sequoiadb.base.Sequoiadb;

/**
 * @author qiushanggao
 *
 */
public abstract class NodeGroup {
	private String name;

	private List<com.sequoiadb.ant.datatype.Node> nodes = new ArrayList<com.sequoiadb.ant.datatype.Node>();
	
	public abstract void start(Sequoiadb sdb) throws BuildException;
	
	public abstract void waitForStart(Sequoiadb sdb, long timeout) throws BuildException;
	
	
	public void setName(String value)
	{
		name = value;
	}
	
	public String getName()
	{
		return name;
	}
	
	public com.sequoiadb.ant.datatype.Node createNode()
	{
		com.sequoiadb.ant.datatype.Node node = new Node();
		nodes.add(node);
		
		return node;
	}
	
	public List<com.sequoiadb.ant.datatype.Node> getNodeList()
	{
		return nodes;
	}
	
	
}
