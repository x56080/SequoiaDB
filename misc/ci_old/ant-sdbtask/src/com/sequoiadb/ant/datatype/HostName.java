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
