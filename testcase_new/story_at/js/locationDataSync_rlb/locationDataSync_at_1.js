/***************************************************************************************************
 * @Description: 验证在Location下增量同步时选择同步源是否正确
 * @ATCaseID: locationSyncData_at_1
 * @Author: Huangyouquan
 * @TestlinkCase: 无
 * @Change    Activity:
 * Date       Who         Description
 * ========== =========== =========================================================
 * 11/28/2022 Huangyouquan  Test data syn select
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：一组三节点的组
 * 测试场景：Location内节点数据相差较近
 * 测试步骤：
 *  1. 取备节点设置location
 *  2. 停节点使Location生效
 *  3. 等待组内节点数据相差不大
 *  4. 会话快照查询节点同步源的选择
 * 期望结果：
 *  节点优先选location内数据量相差不大的primary作为同步源
 **************************************************************************************************/
testConf.skipStandAlone = true;

main(test);
function test() {
  var groupName = "location_dataSync_rlb";
  var location = "locationdatasync_at_1";
  var csName = CHANGEDPREFIX + "locationDataSync_at_1_cs";
  var clName = CHANGEDPREFIX + "locationDataSync_at_1_cl";

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
  var primaryNode = rg.getMaster( primaryNodeName );
  var primaryNodeID = primaryNode.getDetailObj().toObj()["NodeID"] ;
  nodes = getSlaveList(nodes, primaryNodeName);

  if ( nodes.length < 2 )
  {
      return ;
  }

  try {
      db.updateConf( {syncwithlocation:false}, {GroupName:groupName} ) ;
      setLocationForNodes(rg, nodes, location);

      // check sync source node id
      var locPrimaryNodeName = checkAndGetLocationHasPrimary(db, groupName, location, 34);
      println( "Location primary is: " + locPrimaryNodeName ) ;

      nodeID = nodes[1].NodeID;
      rg.reelectLocation( location, {NodeID:nodeID} ) ;

      locPrimaryNodeName = checkAndGetLocationHasPrimary(db, groupName, location, 34);
      println( "After reelect location primary is: " + locPrimaryNodeName ) ;

      sleep( 2000 ) ;

      db.updateConf( {syncwithlocation:true}, {GroupName:groupName} ) ;

      for (i = 0; i < 10 ; i++) {
        cl.insert({ a: i });
      }

      sleep( 2000 ) ;

      checkPeerNodeID(db, nodes[1], primaryNodeID);
      checkPeerNodeID(db, nodes[0], nodeID);

      db.updateConf( {syncwithlocation:false}, {GroupName:groupName} ) ;

      for (i = 0; i < 10 ; i++) {
        cl.insert({ a: i });
      }
      
      sleep( 2000 ) ;

      checkPeerNodeID(db, nodes[1], primaryNodeID);
      checkPeerNodeID(db, nodes[0], primaryNodeID);

  } finally {
      // clear data
      commDropCS(db, csName);
      db.deleteConf( {syncwithlocation:false}, {GroupName:groupName} ) ;
      clearLocationForNodes(rg, nodes);
  }
}
