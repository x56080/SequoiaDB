/**
 * 
 */
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
