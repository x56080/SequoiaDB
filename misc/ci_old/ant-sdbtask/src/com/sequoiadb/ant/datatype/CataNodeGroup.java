/**
 * 
 */
package com.sequoiadb.ant.datatype;



import java.util.LinkedHashMap;
import java.util.Map;

import org.apache.tools.ant.BuildException;
import org.bson.BSONObject;


import com.sequoiadb.base.ReplicaGroup;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;

/**
 * @author qiushanggao
 * 
 */
public class CataNodeGroup extends NodeGroup {

	private static String CATALOG_GROUP_NAME = "SYSCatalogGroup";

	@Override
	public void start(Sequoiadb sdb) throws BuildException {

		ReplicaGroup group = null;
		try {
			setName(CATALOG_GROUP_NAME);
			group = sdb.getReplicaGroup(getName());
		} catch (BaseException e) {
			group = null;
		}

		if (group == null) {
			// group is not exist
			com.sequoiadb.ant.datatype.Node nodeInfo = getNodeList().get(0);
			getNodeList().remove(0);

			try {
				//BSONObject bson = new BasicBSONObject() ;
				//bson.put("SharingBreak", 10000) ;
				//Map< String , String > map = new LinkedHashMap< String , String >() ;
				//map.put("SharingBreak", "10000") ;
				// create group
				System.out.println("create Catalog master node , info is :");
				System.out.println("nodeInfo.getHost() = "+ nodeInfo.getHost()+",\n"
				+ "nodeInfo.getBasePort() = "+ nodeInfo.getBasePort()+",\n"
				+ "nodeInfo.getDbpath() = " + nodeInfo.getDbpath()+",\n"
				+ "nodeInfo.getConfigMap() = " +nodeInfo.getConfigMap());
				System.out.println("create CataRG , \n" +
						"host :"+nodeInfo.getHost()+"\n" +
						"port : " +nodeInfo.getBasePort() +"\n" +
						"path : " + nodeInfo.getDbpath() + "\n" +
						"configMap :"+nodeInfo.getConfigMap().toString() + "\n\n\n"
						);
				sdb.createReplicaCataGroup(nodeInfo.getHost(),
						nodeInfo.getBasePort(), nodeInfo.getDbpath(),
						nodeInfo.getConfigMap() );
				
				
				/*chen write , here seem have propore
				ReplicaGroup cataRG = sdb.getReplicaGroup( CATALOG_GROUP_NAME ) ;
				ReplicaNode cataNode = cataRG.createNode( nodeInfo.getHost() , 
														nodeInfo.getBasePort() , 
														nodeInfo.getDbpath() , 
														nodeInfo.getConfigMap() ) ;
				cataNode.start() ; 
				*/
				try {
					Thread.sleep(2000);
				} catch (InterruptedException e) {
				}
			} catch (BaseException e) {
				// Do nothing
			}
		}

	}

	@Override
	public void waitForStart(Sequoiadb sdb, long timeout) throws BuildException {

		ReplicaGroup group = null;

		// Wait for cata select group.
		int i = 0;
		while (true) {
			try {
				group = sdb.getReplicaGroup(getName());
				if (group != null) {
					break;
				}
			} catch (BaseException baseException) {
				//System.out.println("group="+group);
				// Do nothing
			}

			i++;
			if (i > timeout) {
				throw new BuildException("Group:" + this.getName()
						+ " select master timeout.");
			}

			try {
				Thread.sleep(1000);
			} catch (InterruptedException e) {
			}
		}

		System.out.println("The Group : " + this.getName() + " select master time is " + i + " seconds.");

		com.sequoiadb.base.Node node = null;
		
		//List<ReplicaNode> replNodes = new LinkedList<ReplicaNode>();
		
		for (com.sequoiadb.ant.datatype.Node nodeInfo : getNodeList()) {
			node = group.getNode(nodeInfo.getHost(), nodeInfo.getBasePort());

			if (node == null) {
				System.out.println("host :"+nodeInfo.getHost()+"\n" +
						"port : " +nodeInfo.getBasePort() +"\n" +
						"path : " + nodeInfo.getDbpath() + "\n" +
						"configMap :"+nodeInfo.getConfigMap().toString()
						);
				node = group.createNode(nodeInfo.getHost(),
						nodeInfo.getBasePort(), nodeInfo.getDbpath(),
						nodeInfo.getConfigMap());

				node.start();
				//replNodes.add(node);
			} else {
				throw new BuildException("Node repeat: hostname="
						+ nodeInfo.getHost() + "servicename:"
						+ nodeInfo.getBasePort());
			}
		}
		
		//for (ReplicaNode replNode : replNodes)
		//{
		//	try
		//	{
		//		replNode.start();
		//	}
		//	catch(BaseException e)
		//	{
		//		e.printStackTrace();
		//		throw new BuildException(e.getMessage());
		//	}
		//}

		// Wait start selected master.
		try {
			Thread.sleep(3000);
		} catch (InterruptedException e) {
		}

		// Wait for cata select master.
		i = 0;
		while (true) {
			try {
				node = group.getMaster();
				if (node != null) {
					break;
				}
			} catch (BaseException baseException) {
				// Do nothing
			}

			i++;
			if (i > timeout) {
				throw new BuildException("Group:" + this.getName()
						+ " select master timeout.");
			}

			try {
				Thread.sleep(1000);
			} catch (InterruptedException e) {
			}
		}

		// Wait selected master complete.
		try {
			Thread.sleep(10000);
		} catch (InterruptedException e) {
		}

	}
}
