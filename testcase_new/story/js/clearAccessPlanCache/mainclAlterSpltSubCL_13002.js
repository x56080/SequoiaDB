/************************************
*@Description: 主子表上修改子表为分区表，清空缓存功能验证
*@author:      zhaoyu
*@createdate:  2018.1.26
*@testlinkCase:seqDB-13002
**************************************/
var maincsName = COMMCSNAME + "_maincs_13002"; 
var subcsName1 = COMMCSNAME + "_subcs_13002_1"; 
var mainclName = COMMCLNAME + "_maincl_13002"; 
var subclName1 = COMMCLNAME + "_subcl_13002_1"; 
var mainclFullName = maincsName + "." + mainclName; 
var subclFullName1 = subcsName1 + "." + subclName1; 

var maincl; 

var srcGroupName; 
var desGroupName; 

var insertDiffNum = 4000; 
var insertSameNum = 2000; 

var db1; 
var db2; 
var dbclPrimary; 
var dbclSlave; 


function main()
{
   //独立模式及1组模式不执行该用例
   try
   {
      //判断独立模式
      if( true == commIsStandalone( db ) )
      {
         println( "run mode is standalone" ); 
         return; 
      }
      
      //判断1组模式
      var allGroupName = getGroupName( db ); 
      if( 1 === allGroupName.length )
      {
         println( "only one group" ); 
         return; 
      }
      
      //判断1节点模式
      var groups = new Array(); 
      temp = commGetGroups( db ); 
      for( var i = 0; i < temp.length; i++ )
      {
         groups.push( temp[i][0].GroupName ); 
      }
      
      var nodes = getNodesInGroups( db, groups ); 
      for( var i = 0; i < nodes.length; i++ )
      {
         if( 1 === nodes[i].length )
         {
            println( "group exists one node" ); 
            return; 
         }
      }
   }
   catch( e )
   {
      throw e; 
   }
   
   //清理环境
   commDropCS( db, subcsName1, true, "drop subcs before test" ); 
   commDropCS( db, maincsName, true, "drop maincs before test" ); 
   
   //获取数据组
   var temp = commGetGroups( db ); 
   if( commIsStandalone( db )== false )
   {
      srcGroupName = temp[0][0].GroupName; 
      desGroupName = temp[1][0].GroupName; 
   }
   println( "srcGroupName:" + srcGroupName ); 
   println( "desGroupName:" + desGroupName ); 
   
   //创建主表cl
   var mainclOption = {IsMainCL: true, ShardingKey: {"a": 1}, ShardingType: "range"}; 
   maincl = commCreateCLByOption( db, maincsName, mainclName, mainclOption ); 
   
   //创建子表cl
   var subclOption1 = {Group: srcGroupName}; 
   commCreateCLByOption( db, subcsName1, subclName1, subclOption1 ); 
   
   //attach cl
   maincl.attachCL( subclFullName1, {LowBound: {a:0}, UpBound:{a:4000}} ); 
   
   //创建索引
   commCreateIndex( maincl, "a1", {a1:1} ); 
   
   //插入记录
   insertDiffDatas( maincl, insertDiffNum ); 
   insertSameDatas( maincl, insertSameNum, 0 ); 
   
   //获取主备节点
   db1 = new Sdb( db ); 
   db1.setSessionAttr( { PreferedInstance: "m" } ); 
   dbclPrimary = db1.getCS( maincsName ).getCL( mainclName ); 
   db2 = new Sdb( db ); 
   db2.setSessionAttr( { PreferedInstance: "s" } ); 
   dbclSlave = db2.getCS( maincsName ).getCL( mainclName ); 
   
   //指定主表cl执行统计
   analyze( db, {Collection: mainclFullName} ); 
   
   //检查主备同步
   checkConsistency( db, null, null, [srcGroupName, desGroupName] ); 
   
   //检查统计
   checkStat( db, subcsName1, subclName1, "a1", true, true ); 
   
   //执行查询
   var findConf = {a1:0}; 
   query( dbclPrimary, findConf, null, null, insertSameNum + 1 ); 
   query( dbclSlave, findConf, null, null, insertSameNum + 1 ); 
   
   //检查访问计划快照
   var tmp = [{GroupName:srcGroupName, ScanType:"tbscan", IndexName:""}]; 
   var expAccessPlan = tmp.concat( tmp ); 
   var actAccessPlan = getMainclAccessPlans( db, {Collection: mainclFullName} ); 
   checkMainclAccessPlans( expAccessPlan, actAccessPlan ); 
   
   //修改子表为分区表
   var subcl1 = db.getCS( subcsName1 ).getCL( subclName1 ); 
   subcl1.alter( {ShardingKey:{a0:1}, ShardingType:"range"} ); 
   
   //检查主备同步
   checkConsistency( db, null, null, [srcGroupName, desGroupName] ); 
   
   //检查统计
   checkStat( db, subcsName1, subclName1, "a1", true, true ); 
   
   //检查访问计划快照
   var tmp = [{GroupName:srcGroupName, ScanType:"tbscan", IndexName:""}]; 
   var expAccessPlan = tmp.concat( tmp ); 
   var actAccessPlan = getMainclAccessPlans( db, {Collection: mainclFullName} ); 
   checkMainclAccessPlans( expAccessPlan, actAccessPlan ); 
   
   //子表1执行切分
   split( subcsName1, subclName1, srcGroupName, desGroupName, {a0:2000}, {a0:4000} ); 
   
   //检查访问计划快照
   var tmp = [{GroupName:srcGroupName, ScanType:"tbscan", IndexName:""}]; 
   var expAccessPlan = tmp.concat( tmp ); 
   var actAccessPlan = getMainclAccessPlans( db, {Collection: mainclFullName} ); 
   checkMainclAccessPlans( expAccessPlan, actAccessPlan ); 
   
   //执行查询
   var findConf = {a1:0}; 
   query( dbclPrimary, findConf, null, null, insertSameNum + 1 ); 
   query( dbclSlave, findConf, null, null, insertSameNum + 1 ); 
   
   //检查访问计划快照
   var tmp = [{GroupName:srcGroupName, ScanType:"tbscan", IndexName:""}, 
   {GroupName:desGroupName, ScanType:"ixscan", IndexName:"a1"}]; 
   var expAccessPlan = tmp.concat( tmp ); 
   var actAccessPlan = getMainclAccessPlans( db, {Collection: mainclFullName} ); 
   checkMainclAccessPlans( expAccessPlan, actAccessPlan ); 
   
   //清理环境
   commDropCS( db, subcsName1 ); 
   commDropCS( db, maincsName ); 
   db1.close(); 
   db2.close(); 
   
}
main()
