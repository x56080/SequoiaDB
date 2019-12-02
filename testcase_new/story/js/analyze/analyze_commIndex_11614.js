/************************************
*@Description: 指定普通索引收集统计信息
*@author:      zhaoyu
*@createdate:  2017.11.9
*@testlinkCase:seqDB-11614
**************************************/
function main()
{
   var clName = COMMCLNAME + "_11614"; 
   var clFullName = COMMCSNAME + "." + clName; 
   var insertNum = 2000; 
   var sameValues = 9000; 
   
   //清理环境
   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the beginning" ); 
   
   //创建cl
   var dbcl = commCreateCL( db, COMMCSNAME, clName ); 
   
   //创建索引
   commCreateIndex( dbcl, "a", {a:1} ); 
   commCreateIndex( dbcl, "a1", {a1:1} ); 
   
   //插入记录
   insertDiffDatas( dbcl, insertNum ); 
   insertSameDatas( dbcl, insertNum, sameValues ); 
   
   //获取主备节点
   var db1 = new Sdb( db ); 
   db1.setSessionAttr( { PreferedInstance: "m" } ); 
   var dbclPrimary = db1.getCS( COMMCSNAME ).getCL( clName ); 
   //var db2 = new Sdb( db ); 
   //db2.setSessionAttr( { PreferedInstance: "s" } ); 
   //var dbclSlave = db2.getCS( COMMCSNAME ).getCL( clName ); 
   
   //检查统计信息
   checkConsistency( db, COMMCSNAME, clName ); 
   checkStat( db, COMMCSNAME, clName, "a", false, false ); 
   checkStat( db, COMMCSNAME, clName, "a1", false, false ); 
   
   //检查主备节点访问计划
   var findConf = {a:sameValues}; 
   query( dbclPrimary, findConf, null, null, insertNum ); 
   //query( dbclSlave, findConf, null, null, insertNum ); 
   
   var findConf = {a1:sameValues}; 
   query( dbclPrimary, findConf, null, null, insertNum ); 
   //query( dbclSlave, findConf, null, null, insertNum ); 
   
   //检查访问计划快照
   var expAccessPlan = [{ScanType:"ixscan", IndexName:"a"}, 
   {ScanType:"ixscan", IndexName:"a1"}]; 
   var actAccessPlan = getCommonAccessPlans( db, {Collection: clFullName} ); 
   checkSnapShotAccessPlans( clFullName, expAccessPlan, actAccessPlan ); 
   
   //执行统计
   analyze( db, {Collection: COMMCSNAME + "." + clName, Index: "a"} ); 
   
   //检查统计信息
   checkConsistency( db, COMMCSNAME, clName ); 
   checkStat( db, COMMCSNAME, clName, "a", true, true ); 
   checkStat( db, COMMCSNAME, clName, "a1", true, false ); 
   
   //检查访问计划快照
   var expAccessPlan = []; 
   var actAccessPlan = getCommonAccessPlans( db, {Collection: clFullName} ); 
   checkSnapShotAccessPlans( clFullName, expAccessPlan, actAccessPlan ); 
   
   //执行查询
   var findConf = {a:sameValues}; 
   query( dbclPrimary, findConf, null, null, insertNum ); 
   //query( dbclSlave, findConf, null, null, insertNum ); 
   
   var findConf = {a1:sameValues}; 
   query( dbclPrimary, findConf, null, null, insertNum ); 
   //query( dbclSlave, findConf, null, null, insertNum ); 
   
   //检查访问计划快照
   var expAccessPlan = [{ScanType:"tbscan", IndexName:""}, 
   {ScanType:"ixscan", IndexName:"a1"}]; 
   var actAccessPlan = getCommonAccessPlans( db, {Collection: clFullName} ); 
   checkSnapShotAccessPlans( clFullName, expAccessPlan, actAccessPlan ); 
   
   //执行统计，Mode:2
   //执行统计
   analyze( db, {Mode : 2, Collection: COMMCSNAME + "." + clName, Index: "a"} ); 
   
   //检查统计信息
   checkConsistency( db, COMMCSNAME, clName ); 
   checkStat( db, COMMCSNAME, clName, "a", true, true ); 
   checkStat( db, COMMCSNAME, clName, "a1", true, false ); 
   
   //检查访问计划快照
   var expAccessPlan = []; 
   var actAccessPlan = getCommonAccessPlans( db, {Collection: clFullName} ); 
   checkSnapShotAccessPlans( clFullName, expAccessPlan, actAccessPlan ); 
   
   //执行查询
   var findConf = {a:sameValues}; 
   query( dbclPrimary, findConf, null, null, insertNum ); 
   //query( dbclSlave, findConf, null, null, insertNum ); 
   
   var findConf = {a1:sameValues}; 
   query( dbclPrimary, findConf, null, null, insertNum ); 
   //query( dbclSlave, findConf, null, null, insertNum ); 
   
   //检查访问计划快照
   var expAccessPlan = [{ScanType:"tbscan", IndexName:""}, 
   {ScanType:"ixscan", IndexName:"a1"}]; 
   var actAccessPlan = getCommonAccessPlans( db, {Collection: clFullName} ); 
   checkSnapShotAccessPlans( clFullName, expAccessPlan, actAccessPlan ); 
   
   //删除索引
   commDropIndex( dbcl, "a" ); 
   
   //指定不存在的索引执行统计信息
   try
   {
      db.analyze( {Collection: COMMCSNAME + "." + clName, Index: "a"} ); 
      throw "NEED_ERR"; 
   }
   catch( e )
   {
      if( e !== -264 && e !== -47 )
      {
         throw e; 
      }
   }
   
   //不指定cl但指定索引收集统计信息
   try
   {
      db.analyze( {Index: "a"} ); 
      throw "NEED_ERR"; 
   }
   catch( e )
   {
      if( e !== -6 )
      {
         throw e; 
      }
   }
   
   //清理环境
   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the end" ); 
   db1.close(); 
   //db2.close(); 
   
}
main()
