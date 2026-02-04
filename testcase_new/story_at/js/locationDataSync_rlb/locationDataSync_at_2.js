/***************************************************************************************************
 * @Description: 验证在亲和性Location下增量同步时选择同步源是否正确
 * @ATCaseID: locationSyncData_at_1
 * @Author: Huangyouquan
 * @TestlinkCase: 无
 * @Change    Activity:
 * Date       Who         Description
 * ========== =========== =========================================================
 * 11/28/2022 Huangyouquan  Test data syn select
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：一组三节点以上的组
 * 测试场景：Location内节点数据相差较近
 * 测试步骤：
 *  1. 取俩个备节点设置亲和的location
 *  2. 停节点使Location生效
 *  3. 等待组内节点数据相差不大
 *  4. 会话快照查询节点同步源的选择
 * 期望结果：
 *  节点优先选数据量相差不大亲和性节点作为同步源
 **************************************************************************************************/
testConf.skipStandAlone = true;

main(test);
function test() {
  var groupName = "location_dataSync_rlb";
  var location1 = "locationdatasync_at_2.";

  var csName = CHANGEDPREFIX + "locationDataSync_at_2_cs";
  var clName = CHANGEDPREFIX + "locationDataSync_at_2_cl";

  // insert record
  commCreateCS(db, csName);
  var cl = commCreateCL(db, csName, clName, { Group: groupName });
  data = [];
  for (i = 0; i < 1000; i++) {
    data.push({ a: i });
  }

  // get slave node and set location
  var rg = db.getRG(groupName);
  var nodes = commGetGroupNodes(db, groupName);
  var primaryNodeName = getReplPrimaryName( rg ) ;
  var primaryNode = rg.getNode(primaryNodeName);
  var primaryNodeID = primaryNode.getDetailObj().toObj()["NodeID"] ;

  nodes = getSlaveList(nodes, primaryNodeName);

  if ( nodes.length < 2 )
  {
      return ;
  }

  try {
      db.updateConf( {syncwithlocation:false}, {GroupName:groupName} ) ;

      var node1 = rg.getNode(nodes[0].HostName, nodes[0].svcname);
      var node2 = rg.getNode(nodes[1].HostName, nodes[1].svcname);
      node1.setLocation(location1 + "a");
      node2.setLocation(location1 + "b");

      var loc1PrimaryNodeName = checkAndGetLocationHasPrimary(db, groupName, location1 + "a", 34);
      var loc2PrimaryNodeName = checkAndGetLocationHasPrimary(db, groupName, location1 + "b", 34);

      println( "Location1 primary is: " + loc1PrimaryNodeName ) ;
      println( "Location2 primary is: " + loc2PrimaryNodeName ) ;
      
      sleep( 2000 ) ;

      db.updateConf( {syncwithlocation:true}, {GroupName:groupName} ) ;

      for (i = 0; i < 10; i++) {
        cl.insert({ a: i });
      }

      sleep( 2000 ) ;

      // check sync source node id
      checkPeerNodeID(db, nodes[0], primaryNodeID);
      checkPeerNodeID(db, nodes[1], primaryNodeID);
  } finally {
      // clear data
      commDropCS(db, csName);
      db.deleteConf( {syncwithlocation:false}, {GroupName:groupName} ) ;
      clearLocationForNodes(rg, nodes);
  }
}
