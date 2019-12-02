/************************************
*@Description: : 普通表删除索引，清空缓存功能验证
*@author:      zhaoyu
*@createdate:  2017.11.8
*@testlinkCase:seqDB-12976
**************************************/
function main()
{
   var clName = COMMCLNAME + "12976"; 
   var insertNum = 5000; 
   var sameValues = 9000; 
   
   var clFullName = COMMCSNAME + "." + clName; 
   
   //清理环境
   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the beginning" ); 
   
   //创建cl
   var dbcl = commCreateCL( db, COMMCSNAME, clName ); 
   
   //创建索引
   commCreateIndex( dbcl, "a", {a:1} ); 
   commCreateIndex( dbcl, "b", {b:1} ); 
   
   //插入记录
   insertDiffDatas( dbcl, insertNum ); 
   insertSameDatas( dbcl, insertNum, sameValues ); 
   
   //获取主备节点
   var db1 = new Sdb( db ); 
   db1.setSessionAttr( { PreferedInstance: "m" } ); 
   var dbclPrimary = db1.getCS( COMMCSNAME ).getCL( clName ); 
   var db2 = new Sdb( db ); 
   db2.setSessionAttr( { PreferedInstance: "s" } ); 
   var dbclSlave = db2.getCS( COMMCSNAME ).getCL( clName ); 
   
   //检查主备同步
   checkConsistency( db, COMMCSNAME, clName ); 
   
   //检查统计信息
   checkStat( db, COMMCSNAME, clName, "a", false, false ); 
   checkStat( db, COMMCSNAME, clName, "b", false, false ); 
   
   //分别在主备节点执行查询
   var findConf1 = {a:sameValues}; 
   var findConf2 = {b:sameValues}; 
   
   query( dbclPrimary, findConf1, null, null, insertNum ); 
   query( dbclPrimary, findConf2, null, null, insertNum ); 
   query( dbclSlave, findConf1, null, null, insertNum ); 
   query( dbclSlave, findConf2, null, null, insertNum ); 
   
   //检查主备节点访问计划快照
   var accessFindOption = { Collection: clFullName }; 
   var actAccessPlans = getCommonAccessPlans( db, accessFindOption ); 
   var expAccessPlans = [{ScanType:"ixscan", IndexName:"a"}, 
   {ScanType:"ixscan", IndexName:"b"}, 
   {ScanType:"ixscan", IndexName:"a"}, 
   {ScanType:"ixscan", IndexName:"b"}]; 
   
   checkSnapShotAccessPlans( clFullName, expAccessPlans, actAccessPlans ); 
   
   println( "check result before analyze success!" ); 
   
   //执行统计
   analyze( db, {Collection: COMMCSNAME + "." + clName} ); 
   
   //检查主备同步
   checkConsistency( db, COMMCSNAME, clName ); 
   
   //检查统计信息
   checkStat( db, COMMCSNAME, clName, "a", true, true ); 
   checkStat( db, COMMCSNAME, clName, "b", true, true ); 
   
   //检查主备节点访问计划快照
   var accessFindOption = { Collection: clFullName }; 
   var actAccessPlans = getCommonAccessPlans( db, accessFindOption ); 
   var expAccessPlans = []; 
   checkSnapShotAccessPlans( clFullName, expAccessPlans, actAccessPlans ); 
   
   //分别在主备节点执行查询
   var findConf1 = {a:sameValues}; 
   var findConf2 = {b:sameValues}; 
   
   query( dbclPrimary, findConf1, null, null, insertNum ); 
   query( dbclPrimary, findConf2, null, null, insertNum ); 
   query( dbclSlave, findConf1, null, null, insertNum ); 
   query( dbclSlave, findConf2, null, null, insertNum ); 
   
   //检查主备节点访问计划快照
   var accessFindOption = { Collection: clFullName }; 
   var actAccessPlans = getCommonAccessPlans( db, accessFindOption ); 
   var expAccessPlans = [{ScanType:"tbscan", IndexName:""}, 
   {ScanType:"tbscan", IndexName:""}, 
   {ScanType:"tbscan", IndexName:""}, 
   {ScanType:"tbscan", IndexName:""}]; 
   
   checkSnapShotAccessPlans( clFullName, expAccessPlans, actAccessPlans ); 
   
   println( "check result after analyze success!" ); 
   
   //删除索引
   commDropIndex( dbcl, "a" ); 
   
   //检查主备同步
   checkConsistency( db, COMMCSNAME, clName ); 
   
   //检查统计信息
   checkStat( db, COMMCSNAME, clName, "a", true, false ); 
   checkStat( db, COMMCSNAME, clName, "b", true, true ); 
   
   //检查主备节点访问计划快照
   var accessFindOption = { Collection: clFullName }; 
   var actAccessPlans = getCommonAccessPlans( db, accessFindOption ); 
   var expAccessPlans = []; 
   checkSnapShotAccessPlans( clFullName, expAccessPlans, actAccessPlans ); 
   
   //分别在主备节点执行查询
   var findConf1 = {a:sameValues}; 
   var findConf2 = {b:sameValues}; 
   
   query( dbclPrimary, findConf1, null, null, insertNum ); 
   query( dbclPrimary, findConf2, null, null, insertNum ); 
   query( dbclSlave, findConf1, null, null, insertNum ); 
   query( dbclSlave, findConf2, null, null, insertNum ); 
   
   //检查主备节点访问计划快照
   var accessFindOption = { Collection: clFullName }; 
   var actAccessPlans = getCommonAccessPlans( db, accessFindOption ); 
   var expAccessPlans = [{ScanType:"tbscan", IndexName:""}, 
   {ScanType:"tbscan", IndexName:""}, 
   {ScanType:"tbscan", IndexName:""}, 
   {ScanType:"tbscan", IndexName:""}]; 
   
   checkSnapShotAccessPlans( clFullName, expAccessPlans, actAccessPlans ); 
   
   println( "check result after drop index success!" ); 
   
   //再次创建相同索引
   commCreateIndex( dbcl, "a", {a:1} ); 
   
   //检查主备同步
   checkConsistency( db, COMMCSNAME, clName ); 
   
   //检查统计信息
   checkStat( db, COMMCSNAME, clName, "a", true, false ); 
   checkStat( db, COMMCSNAME, clName, "b", true, true ); 
   
   //检查主备节点访问计划快照
   var accessFindOption = { Collection: clFullName }; 
   var actAccessPlans = getCommonAccessPlans( db, accessFindOption ); 
   var expAccessPlans = []; 
   checkSnapShotAccessPlans( clFullName, expAccessPlans, actAccessPlans ); 
   
   //分别在主备节点执行查询
   var findConf1 = {a:sameValues}; 
   var findConf2 = {b:sameValues}; 
   
   query( dbclPrimary, findConf1, null, null, insertNum ); 
   query( dbclPrimary, findConf2, null, null, insertNum ); 
   query( dbclSlave, findConf1, null, null, insertNum ); 
   query( dbclSlave, findConf2, null, null, insertNum ); 
   
   //检查主备节点访问计划快照
   var accessFindOption = { Collection: clFullName }; 
   var actAccessPlans = getCommonAccessPlans( db, accessFindOption ); 
   var expAccessPlans = [{ScanType:"ixscan", IndexName:"a"}, 
   {ScanType:"tbscan", IndexName:""}, 
   {ScanType:"ixscan", IndexName:"a"}, 
   {ScanType:"tbscan", IndexName:""}]; 
   
   checkSnapShotAccessPlans( clFullName, expAccessPlans, actAccessPlans ); 
   
   println( "check result after create the same index success!" ); 
   
   //清空环境
   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the end" ); 
   db1.close(); 
   db2.close(); 
   
}
main()
