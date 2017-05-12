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
