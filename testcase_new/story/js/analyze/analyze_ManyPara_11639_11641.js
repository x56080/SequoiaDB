/************************************
*@Description: 指定3个参数组合收集统计信息/指定所有参数收集统计信息
*@author:      zhaoyu
*@createdate:  2017.11.14
*@testlinkCase:seqDB-11639/seqDB-11641
**************************************/
function main()
{
   try
   {
      //判断独立模式
      if( true == commIsStandalone( db ) )
      {
         println( "run mode is standalone" ); 
         return; 
      }
   }
   catch( e )
   {
      throw e; 
   }
   
   var csName = COMMCSNAME + "_11639"
   var clName = COMMCLNAME + "_11639"; 
   var clFullName = csName + "." + clName; 
   var insertNum = 2000; 
   var sameValues = 9000; 
   
   var findConf = {a:sameValues}; 
   var findConf = {a:sameValues}; 
   var expAccessPlan1 = [{ScanType:"tbscan", IndexName:""}]; 
   var expAccessPlan2 = [{ScanType:"ixscan", IndexName:"a"}]; 
   var expAccessPlan3 = []; 
   
   //清理环境
   commDropCS( db, csName, true, "drop cs before test" ); 
   
   //创建cl
   var dbcl = commCreateCL( db, csName, clName ); 
   
   //创建索引
   commCreateIndex( dbcl, "a", {a:1} ); 
   
   //插入记录
   insertDiffDatas( dbcl, insertNum ); 
   insertSameDatas( dbcl, insertNum, sameValues ); 
   
   //获取主备节点
   var db1 = new Sdb( db ); 
   db1.setSessionAttr( { PreferedInstance: "m" } ); 
   var dbclPrimary = db1.getCS( csName ).getCL( clName ); 
   //var db2 = new Sdb( db ); 
   //db2.setSessionAttr( { PreferedInstance: "s" } ); 
   //var dbclSlave = db2.getCS( csName ).getCL( clName ); 
   
   //检查统计信息
   checkConsistency( db, csName, clName ); 
   checkStat( db, csName, clName, "a", false, false ); 
   
   //执行查询
   query( dbclPrimary, findConf, null, null, insertNum ); 
   //query( dbclSlave, findConf, null, null, insertNum ); 
   
   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, {Collection: clFullName} ); 
   checkSnapShotAccessPlans( clFullName, expAccessPlan2, actAccessPlan ); 
   
   //指定cl + index + group执行统计
   var groupName = getSrcGroup( csName, clName ); 
   analyze( db, {Collection: csName + "." + clName, Index: "a", GroupName: groupName} ); 
   
   //检查统计信息
   checkConsistency( db, csName, clName ); 
   checkStat( db, csName, clName, "a", true, true ); 
   
   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, {Collection: clFullName} ); 
   checkSnapShotAccessPlans( clFullName, expAccessPlan3, actAccessPlan ); 
   
   //执行查询
   query( dbclPrimary, findConf, null, null, insertNum ); 
   //query( dbclSlave, findConf, null, null, insertNum ); 
   
   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, {Collection: clFullName} ); 
   checkSnapShotAccessPlans( clFullName, expAccessPlan1, actAccessPlan ); 
   
   //清空统计信息
   analyze( db, {Mode:3, Collection: csName + "." + clName} ); 
   
   //指定cs + group + node执行统计
   var primaryNode = db.getRG( groupName ).getMaster(); 
   var nodeId = parseInt( primaryNode.getNodeDetail().split( ":" )[0] ); 
   println( "nodeId:" + nodeId ); 
   analyze( db, {CollectionSpace: csName, GroupName: groupName, NodeID: nodeId} ); 
   
   //检查统计信息
   checkConsistency( db, csName, clName ); 
   checkStat( db, csName, clName, "a", true, true ); 
   
   //执行查询
   query( dbclPrimary, findConf, null, null, insertNum ); 
   //query( dbclSlave, findConf, null, null, insertNum ); 
   
   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, {Collection: clFullName} ); 
   checkSnapShotAccessPlans( clFullName, expAccessPlan1, actAccessPlan ); 
   
   //清空统计信息
   analyze( db, {Mode:3, Collection: csName + "." + clName} ); 
   
   //指定cl + group + node执行统计
   analyze( db, {Collection: csName + "." + clName, GroupName: groupName, NodeID: nodeId} ); 
   
   //检查统计信息
   checkConsistency( db, csName, clName ); 
   checkStat( db, csName, clName, "a", true, true ); 
   
   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, {Collection: clFullName} ); 
   checkSnapShotAccessPlans( clFullName, expAccessPlan3, actAccessPlan ); 
   
   //执行查询
   query( dbclPrimary, findConf, null, null, insertNum ); 
   //query( dbclSlave, findConf, null, null, insertNum ); 
   
   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, {Collection: clFullName} ); 
   checkSnapShotAccessPlans( clFullName, expAccessPlan1, actAccessPlan ); 
   
   //清空统计信息
   analyze( db, {Mode:3, Collection: csName + "." + clName} ); 
   
   //指定cl + index + node执行统计
   analyze( db, {Collection: csName + "." + clName, Index: "a", NodeID: nodeId} ); 
   
   //检查统计信息
   checkConsistency( db, csName, clName ); 
   checkStat( db, csName, clName, "a", true, true ); 
   
   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, {Collection: clFullName} ); 
   checkSnapShotAccessPlans( clFullName, expAccessPlan3, actAccessPlan ); 
   
   //执行查询
   query( dbclPrimary, findConf, null, null, insertNum ); 
   //query( dbclSlave, findConf, null, null, insertNum ); 
   
   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, {Collection: clFullName} ); 
   checkSnapShotAccessPlans( clFullName, expAccessPlan1, actAccessPlan ); 
   
   //清空统计信息
   analyze( db, {Mode:3, Collection: csName + "." + clName} ); 
   
   //指定cl + index + group + node执行统计, seqDB-11641
   analyze( db, {Collection: csName + "." + clName, Index: "a", GroupName: groupName, NodeID: nodeId} ); 
   
   //检查统计信息
   checkConsistency( db, csName, clName ); 
   checkStat( db, csName, clName, "a", true, true ); 
   
   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, {Collection: clFullName} ); 
   checkSnapShotAccessPlans( clFullName, expAccessPlan3, actAccessPlan ); 
   
   //执行查询
   query( dbclPrimary, findConf, null, null, insertNum ); 
   //query( dbclSlave, findConf, null, null, insertNum ); 
   
   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, {Collection: clFullName} ); 
   checkSnapShotAccessPlans( clFullName, expAccessPlan1, actAccessPlan ); 
   
   //清空统计信息
   analyze( db, {Mode:3, Collection: csName + "." + clName} ); 
   
   //指定不支持的组合参数执行统计
   analyzeInvalidPara( db, {CollectionSpace: csName, Collection: csName + "." + clName, Index: "a"} ); 
   println( "check result after analyze set cs+cl+index success!" ); 
   analyzeInvalidPara( db, {CollectionSpace: csName, Collection: csName + "." + clName, GroupName: groupName} ); 
   println( "check result after analyze set cs+cl+group success!" ); 
   analyzeInvalidPara( db, {CollectionSpace: csName, Collection: csName + "." + clName, NodeID: nodeId} ); 
   println( "check result after analyze set cs+cl+node success!" ); 
   analyzeInvalidPara( db, {CollectionSpace: csName, Index: "a", GroupName: groupName} ); 
   println( "check result after analyze set cs+index+group success!" ); 
   analyzeInvalidPara( db, {CollectionSpace: csName, Index: "a", NodeID: nodeId} ); 
   println( "check result after analyze set cs+index+node success!" ); 
   analyzeInvalidPara( db, {Index: "a", GroupName: groupName, NodeID: nodeId} ); 
   println( "check result after analyze set index+group+node success!" ); 
   
   //seqDB-11641
   analyzeInvalidPara( db, {CollectionSpace: csName, Collection: csName + "." + clName, Index: "a", GroupName: groupName} ); 
   println( "check result after analyze set cs+cl+index+group success!" ); 
   analyzeInvalidPara( db, {CollectionSpace: csName, Collection: csName + "." + clName, Index: "a", NodeID: nodeId} ); 
   println( "check result after analyze set cs+cl+index+node success!" ); 
   analyzeInvalidPara( db, {CollectionSpace: csName, Collection: csName + "." + clName, GroupName: groupName, NodeID: nodeId} ); 
   println( "check result after analyze set cs+cl+group+node success!" ); 
   
   //清理环境
   commDropCS( db, csName ); 
   db1.close(); 
   //db2.close(); 
   
}
main()

function analyzeInvalidPara( db, options )
{
   try
   {
      db.analyze( options ); 
      throw "NEED_ERR"; 
   }
   catch( e )
   {
      if( e !== -6 )
      {
         throw e; 
      }
   }
}
