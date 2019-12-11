/************************************
*@Description: 指定主表所在的cs收集统计信息
*@author:      zhaoyu
*@createdate:  2017.11.9
*@testlinkCase:seqDB-11610
**************************************/
var maincsName = COMMCSNAME + "_maincs_11610";
var subcsName1 = COMMCSNAME + "_subcs_11610_1";
var subcsName2 = COMMCSNAME + "_subcs_11610_2";
var mainclName = COMMCLNAME + "_maincl_11610";
var subclName1 = COMMCLNAME + "_subcl_11610_1";
var subclName2 = COMMCLNAME + "_subcl_11610_2";
var subclName3 = COMMCLNAME + "_subcl_11610_3";
var subclName4 = COMMCLNAME + "_subcl_11610_4";
var mainclFullName = maincsName + "." + mainclName;
var subclFullName1 = maincsName + "." + subclName1;
var subclFullName2 = maincsName + "." + subclName2;
var subclFullName3 = subcsName1 + "." + subclName3;
var subclFullName4 = subcsName1 + "." + subclName4;

var maincl;

var srcGroupName;
var desGroupName;

var insertDiffNum = 16000;
var insertSameNum = 2000;

var db1;
var db2;
var dbclPrimary;
var dbclSlave;


function main ()
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
   }
   catch( e )
   {
      throw e;
   }

   //清理环境
   commDropCS( db, subcsName1, true, "drop subcs before test" );
   commDropCS( db, subcsName2, true, "drop subcs before test" );
   commDropCS( db, maincsName, true, "drop maincs before test" );

   //获取数据组
   var temp = commGetGroups( db );
   if( commIsStandalone( db ) == false )
   {
      srcGroupName = temp[0][0].GroupName;
      desGroupName = temp[1][0].GroupName;
   }
   println( "srcGroupName:" + srcGroupName );
   println( "desGroupName:" + desGroupName );

   //创建主表cl
   var mainclOption = { IsMainCL: true, ShardingKey: { "a": 1 }, ShardingType: "range" };
   maincl = commCreateCLByOption( db, maincsName, mainclName, mainclOption );

   //创建子表cl
   var subclOption1 = { ShardingKey: { "a0": 1 }, ShardingType: "range", Group: srcGroupName };
   commCreateCLByOption( db, maincsName, subclName1, subclOption1 );
   var subclOption2 = { ShardingKey: { "a0": 1 }, ShardingType: "hash", Group: srcGroupName };
   commCreateCLByOption( db, maincsName, subclName2, subclOption2 );
   commCreateCLByOption( db, subcsName1, subclName3, subclOption1 );
   commCreateCLByOption( db, subcsName1, subclName4, subclOption2 );

   //子表切分
   split( maincsName, subclName1, srcGroupName, desGroupName, { a0: 2000 }, { a0: 4000 } );
   split( maincsName, subclName2, srcGroupName, desGroupName, 50, null );
   split( subcsName1, subclName3, srcGroupName, desGroupName, { a0: 10000 }, { a0: 12000 } );
   split( subcsName1, subclName4, srcGroupName, desGroupName, 50, null );

   //attach cl
   maincl.attachCL( subclFullName2, { LowBound: { a: 4000 }, UpBound: { a: 8000 } } );
   maincl.attachCL( subclFullName4, { LowBound: { a: 12000 }, UpBound: { a: 16000 } } );
   maincl.attachCL( subclFullName1, { LowBound: { a: 0 }, UpBound: { a: 4000 } } );
   maincl.attachCL( subclFullName3, { LowBound: { a: 8000 }, UpBound: { a: 12000 } } );

   //创建索引
   commCreateIndex( maincl, "a1", { a1: 1 } );

   //插入记录
   insertDiffDatas( maincl, insertDiffNum );
   insertSameDatas( maincl, insertSameNum, 0 );
   insertSameDatas( maincl, insertSameNum, 10000 );

   //获取主备节点
   db1 = new Sdb( db );
   db1.setSessionAttr( { PreferedInstance: "m" } );
   dbclPrimary = db1.getCS( maincsName ).getCL( mainclName );
   //db2 = new Sdb( db ); 
   //db2.setSessionAttr( { PreferedInstance: "s" } ); 
   //dbclSlave = db2.getCS( maincsName ).getCL( mainclName ); 

   //检查主备同步
   checkConsistency( db, null, null, [srcGroupName, desGroupName] );

   //检查统计信息
   checkStats( db, maincsName, [subclName1, subclName2], ["$shard", "a1"], false, false, [srcGroupName, desGroupName] );
   checkStats( db, subcsName1, [subclName3, subclName4], ["$shard", "a1"], false, false, [srcGroupName, desGroupName] );

   //主备节点上检查访问计划
   checkExplainBeforeAnalyze();
   println( "---check all explain before analyze success" );

   //执行查询
   var findConf = { a: { $in: [0, 10000] } };
   query( dbclPrimary, findConf, null, null, ( insertSameNum + 1 ) * 2 );
   //query( dbclSlave, findConf, null, null, ( insertSameNum + 1 )*2 ); 
   var findConf = { a0: { $in: [0, 10000] } };
   query( dbclPrimary, findConf, null, null, ( insertSameNum + 1 ) * 2 );
   //query( dbclSlave, findConf, null, null, ( insertSameNum + 1 )*2 ); 
   var findConf = { a1: { $in: [0, 10000] } };
   query( dbclPrimary, findConf, null, null, ( insertSameNum + 1 ) * 2 );
   //query( dbclSlave, findConf, null, null, ( insertSameNum + 1 )*2 ); 

   //检查访问计划快照
   var expAccessPlan = [{ GroupName: srcGroupName, ScanType: "ixscan", IndexName: "$shard" },
   { GroupName: srcGroupName, ScanType: "tbscan", IndexName: "" },
   { GroupName: srcGroupName, ScanType: "ixscan", IndexName: "a1" },
   { GroupName: desGroupName, ScanType: "ixscan", IndexName: "$shard" },
   { GroupName: desGroupName, ScanType: "tbscan", IndexName: "" },
   { GroupName: desGroupName, ScanType: "ixscan", IndexName: "a1" }];
   //var expAccessPlan = tmp.concat( tmp ); 
   var actAccessPlan = getMainclAccessPlans( db, { Collection: mainclFullName } );
   checkMainclAccessPlans( expAccessPlan, actAccessPlan );

   //指定主表cs执行统计
   analyze( db, { CollectionSpace: maincsName } );

   //检查主备同步
   checkConsistency( db, null, null, [srcGroupName, desGroupName] );

   //检查统计
   checkStats( db, maincsName, [subclName1, subclName2], "$shard", true, true, [srcGroupName, desGroupName] );
   checkStats( db, subcsName1, [subclName3, subclName4], "$shard", false, false, [srcGroupName, desGroupName] );
   checkStats( db, maincsName, [subclName1, subclName2], "a1", true, true, [srcGroupName, desGroupName] );
   checkStats( db, subcsName1, [subclName3, subclName4], "a1", false, false, [srcGroupName, desGroupName] );

   //检查访问计划快照
   var expAccessPlan = [];
   var actAccessPlan = getMainclAccessPlans( db, { Collection: mainclFullName } );
   checkMainclAccessPlans( expAccessPlan, actAccessPlan );

   //执行查询
   var findConf = { a: { $in: [0, 10000] } };
   query( dbclPrimary, findConf, null, null, ( insertSameNum + 1 ) * 2 );
   //query( dbclSlave, findConf, null, null, ( insertSameNum + 1 )*2 ); 
   var findConf = { a0: { $in: [0, 10000] } };
   query( dbclPrimary, findConf, null, null, ( insertSameNum + 1 ) * 2 );
   //query( dbclSlave, findConf, null, null, ( insertSameNum + 1 )*2 ); 
   var findConf = { a1: { $in: [0, 10000] } };
   query( dbclPrimary, findConf, null, null, ( insertSameNum + 1 ) * 2 );
   //query( dbclSlave, findConf, null, null, ( insertSameNum + 1 )*2 ); 

   //检查访问计划快照
   var expAccessPlan = [{ GroupName: srcGroupName, ScanType: "ixscan", IndexName: "$shard" },
   { GroupName: srcGroupName, ScanType: "tbscan", IndexName: "" },
   { GroupName: srcGroupName, ScanType: "ixscan", IndexName: "a1" },
   { GroupName: desGroupName, ScanType: "ixscan", IndexName: "$shard" },
   { GroupName: desGroupName, ScanType: "tbscan", IndexName: "" },
   { GroupName: desGroupName, ScanType: "ixscan", IndexName: "a1" }];
   //var expAccessPlan = tmp.concat( tmp ); 
   var actAccessPlan = getMainclAccessPlans( db, { Collection: mainclFullName } );
   checkMainclAccessPlans( expAccessPlan, actAccessPlan );

   //主备节点上检查访问计划
   checkExplainAfterAnalyzeMaincs();
   println( "---check all explain after anlyze maincs success" );

   //指定主表cs执行统计
   analyze( db, { Mode: 2, CollectionSpace: maincsName } );

   //检查主备同步
   checkConsistency( db, null, null, [srcGroupName, desGroupName] );

   //检查统计
   checkStats( db, maincsName, [subclName1, subclName2], "$shard", true, true, [srcGroupName, desGroupName] );
   checkStats( db, subcsName1, [subclName3, subclName4], "$shard", false, false, [srcGroupName, desGroupName] );
   checkStats( db, maincsName, [subclName1, subclName2], "a1", true, true, [srcGroupName, desGroupName] );
   checkStats( db, subcsName1, [subclName3, subclName4], "a1", false, false, [srcGroupName, desGroupName] );

   //检查访问计划快照
   var expAccessPlan = [];
   var actAccessPlan = getMainclAccessPlans( db, { Collection: mainclFullName } );
   checkMainclAccessPlans( expAccessPlan, actAccessPlan );

   //执行查询
   var findConf = { a: { $in: [0, 10000] } };
   query( dbclPrimary, findConf, null, null, ( insertSameNum + 1 ) * 2 );
   //query( dbclSlave, findConf, null, null, ( insertSameNum + 1 )*2 ); 
   var findConf = { a0: { $in: [0, 10000] } };
   query( dbclPrimary, findConf, null, null, ( insertSameNum + 1 ) * 2 );
   //query( dbclSlave, findConf, null, null, ( insertSameNum + 1 )*2 ); 
   var findConf = { a1: { $in: [0, 10000] } };
   query( dbclPrimary, findConf, null, null, ( insertSameNum + 1 ) * 2 );
   //query( dbclSlave, findConf, null, null, ( insertSameNum + 1 )*2 ); 

   //检查访问计划快照
   var expAccessPlan = [{ GroupName: srcGroupName, ScanType: "ixscan", IndexName: "$shard" },
   { GroupName: srcGroupName, ScanType: "tbscan", IndexName: "" },
   { GroupName: srcGroupName, ScanType: "ixscan", IndexName: "a1" },
   { GroupName: desGroupName, ScanType: "ixscan", IndexName: "$shard" },
   { GroupName: desGroupName, ScanType: "tbscan", IndexName: "" },
   { GroupName: desGroupName, ScanType: "ixscan", IndexName: "a1" }];
   //var expAccessPlan = tmp.concat( tmp ); 
   var actAccessPlan = getMainclAccessPlans( db, { Collection: mainclFullName } );
   checkMainclAccessPlans( expAccessPlan, actAccessPlan );

   //主备节点上检查访问计划
   checkExplainAfterAnalyzeMaincs();
   println( "---check all explain after anlyze maincs success" );

   //先detach再attach的子表落在主表cs上
   maincl.detachCL( subclFullName1 );
   maincl.attachCL( subclFullName1, { LowBound: { a: 0 }, UpBound: { a: 4000 } } );

   //检查主备同步
   checkConsistency( db, null, null, [srcGroupName, desGroupName] );

   //检查统计信息
   checkStats( db, maincsName, [subclName1, subclName2], "$shard", true, true, [srcGroupName, desGroupName] );
   checkStats( db, subcsName1, [subclName3, subclName4], "$shard", false, false, [srcGroupName, desGroupName] );
   checkStats( db, maincsName, [subclName1, subclName2], "a1", true, true, [srcGroupName, desGroupName] );
   checkStats( db, subcsName1, [subclName3, subclName4], "a1", false, false, [srcGroupName, desGroupName] );

   //检查访问计划快照
   var expAccessPlan = [];
   var actAccessPlan = getMainclAccessPlans( db, { Collection: mainclFullName } );
   checkMainclAccessPlans( expAccessPlan, actAccessPlan );

   //执行查询
   var findConf = { a: { $in: [0, 10000] } };
   query( dbclPrimary, findConf, null, null, ( insertSameNum + 1 ) * 2 );
   //query( dbclSlave, findConf, null, null, ( insertSameNum + 1 )*2 ); 
   var findConf = { a0: { $in: [0, 10000] } };
   query( dbclPrimary, findConf, null, null, ( insertSameNum + 1 ) * 2 );
   //query( dbclSlave, findConf, null, null, ( insertSameNum + 1 )*2 ); 
   var findConf = { a1: { $in: [0, 10000] } };
   query( dbclPrimary, findConf, null, null, ( insertSameNum + 1 ) * 2 );
   //query( dbclSlave, findConf, null, null, ( insertSameNum + 1 )*2 ); 

   //检查访问计划快照
   var expAccessPlan = [{ GroupName: srcGroupName, ScanType: "ixscan", IndexName: "$shard" },
   { GroupName: srcGroupName, ScanType: "tbscan", IndexName: "" },
   { GroupName: srcGroupName, ScanType: "ixscan", IndexName: "a1" },
   { GroupName: desGroupName, ScanType: "ixscan", IndexName: "$shard" },
   { GroupName: desGroupName, ScanType: "tbscan", IndexName: "" },
   { GroupName: desGroupName, ScanType: "ixscan", IndexName: "a1" }];
   //var expAccessPlan = tmp.concat( tmp ); 
   var actAccessPlan = getMainclAccessPlans( db, { Collection: mainclFullName } );
   checkMainclAccessPlans( expAccessPlan, actAccessPlan );

   //主备节点上检查访问计划
   checkExplainAfterAnalyzeMaincs();
   println( "---check all explain after detach/attach cl in maincs success" );

   //指定子表所在cs收集统计
   analyze( db, { CollectionSpace: subcsName1 } );

   //检查主备同步
   checkConsistency( db, null, null, [srcGroupName, desGroupName] );

   //检查统计信息
   checkStats( db, maincsName, [subclName1, subclName2], ["$shard", "a1"], true, true, [srcGroupName, desGroupName] );
   checkStats( db, subcsName1, [subclName3, subclName4], ["$shard", "a1"], true, true, [srcGroupName, desGroupName] );

   //检查访问计划快照
   var expAccessPlan = [{ GroupName: srcGroupName, ScanType: "ixscan", IndexName: "$shard" },
   { GroupName: srcGroupName, ScanType: "tbscan", IndexName: "" },
   { GroupName: srcGroupName, ScanType: "ixscan", IndexName: "a1" },
   { GroupName: desGroupName, ScanType: "ixscan", IndexName: "$shard" },
   { GroupName: desGroupName, ScanType: "tbscan", IndexName: "" },
   { GroupName: desGroupName, ScanType: "ixscan", IndexName: "a1" }];
   //var expAccessPlan = tmp.concat( tmp ); 
   var actAccessPlan = getMainclAccessPlans( db, { Collection: mainclFullName } );
   checkMainclAccessPlans( expAccessPlan, actAccessPlan );

   //主备节点上检查访问计划
   checkExplainAfterAnalyzeMaincs();
   println( "---check all explain after anlyze subcs success" );

   //先detach再attach的子表落在子表cs上
   maincl.detachCL( subclFullName3 );
   maincl.attachCL( subclFullName3, { LowBound: { a: 8000 }, UpBound: { a: 12000 } } );

   //检查主备同步
   checkConsistency( db, null, null, [srcGroupName, desGroupName] );

   //检查统计信息
   checkStats( db, maincsName, [subclName1, subclName2], ["$shard", "a1"], true, true, [srcGroupName, desGroupName] );
   checkStats( db, subcsName1, [subclName3, subclName4], ["$shard", "a1"], true, true, [srcGroupName, desGroupName] );

   //检查访问计划快照
   var expAccessPlan = [];
   var actAccessPlan = getMainclAccessPlans( db, { Collection: mainclFullName } );
   checkMainclAccessPlans( expAccessPlan, actAccessPlan );

   //主备节点上检查访问计划
   checkExplainAfterAnalyzesubcs();
   println( "---check all explain after detach/attach cl in subcs success" );

   //detach所有子表
   maincl.detachCL( subclFullName1 );
   maincl.detachCL( subclFullName2 );
   maincl.detachCL( subclFullName3 );
   maincl.detachCL( subclFullName4 );

   //指定主表执行统计
   analyze( db, { CollectionSpace: maincsName } );

   //检查主备同步
   checkConsistency( db, null, null, [srcGroupName, desGroupName] );

   //检查统计信息
   checkStats( db, maincsName, [subclName1, subclName2], ["$shard", "a1"], true, true, [srcGroupName, desGroupName] );
   checkStats( db, subcsName1, [subclName3, subclName4], ["$shard", "a1"], true, true, [srcGroupName, desGroupName] );

   //清理环境
   commDropCS( db, subcsName1 );
   commDropCS( db, subcsName2 );
   commDropCS( db, maincsName );
   db1.close();
   //db2.close(); 
}
try
{
   main();
}
catch( e )
{
   if( e.constructor === Error )
   {
      println( e.stack );
   }
   throw e;
}


function checkExplainBeforeAnalyze ()
{
   //主表分区键查询
   var findConf = { a: { $in: [0, 10000] } };
   var expExplains = [{ GroupName: srcGroupName, Name: subclFullName1, ScanType: "tbscan", IndexName: "", ReturnNum: insertSameNum + 1 },
   { GroupName: desGroupName, Name: subclFullName1, ScanType: "tbscan", IndexName: "", ReturnNum: 0 },
   { GroupName: srcGroupName, Name: subclFullName3, ScanType: "tbscan", IndexName: "", ReturnNum: 0 },
   { GroupName: desGroupName, Name: subclFullName3, ScanType: "tbscan", IndexName: "", ReturnNum: insertSameNum + 1 }];
   var actExplains = getMainclExplain( dbclPrimary, findConf );
   checkExplain( actExplains, expExplains );

   //var actExplains = getMainclExplain( dbclSlave, findConf ); 
   //checkExplain( actExplains, expExplains ); 

   println( "check maincl key before analyze success!" );

   //子表分区键查询
   var findConf = { a0: { $in: [0, 10000] } };
   var expExplains = [
      {
         GroupName: srcGroupName, Name: subclFullName1,
         ScanType: "ixscan", IndexName: "$shard", ReturnNum: insertSameNum + 1
      },
      {
         GroupName: srcGroupName, Name: subclFullName2,
         ScanType: "ixscan", IndexName: "$shard", ReturnNum: 0
      },
      {
         GroupName: srcGroupName, Name: subclFullName3,
         ScanType: "ixscan", IndexName: "$shard", ReturnNum: 0
      },
      {
         GroupName: srcGroupName, Name: subclFullName4,
         ScanType: "ixscan", IndexName: "$shard", ReturnNum: 0
      },
      {
         GroupName: desGroupName, Name: subclFullName2,
         ScanType: "ixscan", IndexName: "$shard", ReturnNum: 0
      },
      {
         GroupName: desGroupName, Name: subclFullName3,
         ScanType: "ixscan", IndexName: "$shard", ReturnNum: insertSameNum + 1
      },
      {
         GroupName: desGroupName, Name: subclFullName4,
         ScanType: "ixscan", IndexName: "$shard", ReturnNum: 0
      }];

   var actExplains = getMainclExplain( dbclPrimary, findConf );
   checkExplain( actExplains, expExplains );

   //var actExplains = getMainclExplain( dbclSlave, findConf ); 
   //checkExplain( actExplains, expExplains ); 

   println( "check subcl key before analyze success!" );

   //索引键查询
   var findConf = { a1: { $in: [0, 10000] } };
   var expExplains = [
      {
         GroupName: srcGroupName, Name: subclFullName1,
         ScanType: "ixscan", IndexName: "a1", ReturnNum: insertSameNum + 1
      },
      {
         GroupName: desGroupName, Name: subclFullName1,
         ScanType: "ixscan", IndexName: "a1", ReturnNum: 0
      },
      {
         GroupName: srcGroupName, Name: subclFullName2,
         ScanType: "ixscan", IndexName: "a1", ReturnNum: 0
      },
      {
         GroupName: desGroupName, Name: subclFullName2,
         ScanType: "ixscan", IndexName: "a1", ReturnNum: 0
      },
      {
         GroupName: srcGroupName, Name: subclFullName3,
         ScanType: "ixscan", IndexName: "a1", ReturnNum: 0
      },
      {
         GroupName: desGroupName, Name: subclFullName3,
         ScanType: "ixscan", IndexName: "a1", ReturnNum: insertSameNum + 1
      },
      {
         GroupName: srcGroupName, Name: subclFullName4,
         ScanType: "ixscan", IndexName: "a1", ReturnNum: 0
      },
      {
         GroupName: desGroupName, Name: subclFullName4,
         ScanType: "ixscan", IndexName: "a1", ReturnNum: 0
      }];
   var actExplains = getMainclExplain( dbclPrimary, findConf );
   checkExplain( actExplains, expExplains );

   //var actExplains = getMainclExplain( dbclSlave, findConf ); 
   //checkExplain( actExplains, expExplains ); 

   println( "check index before analyze success!" );
}

function checkExplainAfterAnalyzeMaincs ()
{
   //主表分区键查询
   var findConf = { a: { $in: [0, 10000] } };
   var expExplains = [
      {
         GroupName: srcGroupName, Name: subclFullName1,
         ScanType: "tbscan", IndexName: "", ReturnNum: insertSameNum + 1
      },
      {
         GroupName: desGroupName, Name: subclFullName1,
         ScanType: "tbscan", IndexName: "", ReturnNum: 0
      },
      {
         GroupName: srcGroupName, Name: subclFullName3,
         ScanType: "tbscan", IndexName: "", ReturnNum: 0
      },
      {
         GroupName: desGroupName, Name: subclFullName3,
         ScanType: "tbscan", IndexName: "", ReturnNum: insertSameNum + 1
      }];

   var actExplains = getMainclExplain( dbclPrimary, findConf );
   checkExplain( actExplains, expExplains );

   //var actExplains = getMainclExplain( dbclSlave, findConf ); 
   //checkExplain( actExplains, expExplains ); 

   println( "check maincl key after analyze maincs success!" );

   //子表分区键查询
   var findConf = { a0: { $in: [0, 10000] } };
   var expExplains = [
      {
         GroupName: srcGroupName, Name: subclFullName1,
         ScanType: "tbscan", IndexName: "", ReturnNum: insertSameNum + 1
      },
      {
         GroupName: srcGroupName, Name: subclFullName2,
         ScanType: "ixscan", IndexName: "$shard", ReturnNum: 0
      },
      {
         GroupName: srcGroupName, Name: subclFullName3,
         ScanType: "ixscan", IndexName: "$shard", ReturnNum: 0
      },
      {
         GroupName: srcGroupName, Name: subclFullName4,
         ScanType: "ixscan", IndexName: "$shard", ReturnNum: 0
      },
      {
         GroupName: desGroupName, Name: subclFullName2,
         ScanType: "ixscan", IndexName: "$shard", ReturnNum: 0
      },
      {
         GroupName: desGroupName, Name: subclFullName3,
         ScanType: "ixscan", IndexName: "$shard", ReturnNum: insertSameNum + 1
      },
      {
         GroupName: desGroupName, Name: subclFullName4,
         ScanType: "ixscan", IndexName: "$shard", ReturnNum: 0
      }];

   var actExplains = getMainclExplain( dbclPrimary, findConf );
   checkExplain( actExplains, expExplains );

   //var actExplains = getMainclExplain( dbclSlave, findConf ); 
   //checkExplain( actExplains, expExplains ); 

   println( "check subcl key after analyze maincs success!" );

   //索引键查询
   var findConf = { a1: { $in: [0, 10000] } };
   var expExplains = [
      {
         GroupName: srcGroupName, Name: subclFullName1,
         ScanType: "tbscan", IndexName: "", ReturnNum: insertSameNum + 1
      },
      {
         GroupName: desGroupName, Name: subclFullName1,
         ScanType: "ixscan", IndexName: "a1", ReturnNum: 0
      },
      {
         GroupName: srcGroupName, Name: subclFullName2,
         ScanType: "ixscan", IndexName: "a1", ReturnNum: 0
      },
      {
         GroupName: desGroupName, Name: subclFullName2,
         ScanType: "ixscan", IndexName: "a1", ReturnNum: 0
      },
      {
         GroupName: srcGroupName, Name: subclFullName3,
         ScanType: "ixscan", IndexName: "a1", ReturnNum: 0
      },
      {
         GroupName: desGroupName, Name: subclFullName3,
         ScanType: "ixscan", IndexName: "a1", ReturnNum: insertSameNum + 1
      },
      {
         GroupName: srcGroupName, Name: subclFullName4,
         ScanType: "ixscan", IndexName: "a1", ReturnNum: 0
      },
      {
         GroupName: desGroupName, Name: subclFullName4,
         ScanType: "ixscan", IndexName: "a1", ReturnNum: 0
      }];

   var actExplains = getMainclExplain( dbclPrimary, findConf );
   checkExplain( actExplains, expExplains );

   //var actExplains = getMainclExplain( dbclSlave, findConf ); 
   //checkExplain( actExplains, expExplains ); 

   println( "check index after analyze maincs success!" );
}

function checkExplainAfterAnalyzesubcs ()
{
   //查询最终落在子表cs上，检查主备节点访问计划
   //主表分区键查询
   var findConf = { a: { $in: [0, 10000] } };
   var expExplains = [
      {
         GroupName: srcGroupName, Name: subclFullName1,
         ScanType: "tbscan", IndexName: "", ReturnNum: insertSameNum + 1
      },
      {
         GroupName: desGroupName, Name: subclFullName1,
         ScanType: "tbscan", IndexName: "", ReturnNum: 0
      },
      {
         GroupName: srcGroupName, Name: subclFullName3,
         ScanType: "tbscan", IndexName: "", ReturnNum: 0
      },
      {
         GroupName: desGroupName, Name: subclFullName3,
         ScanType: "tbscan", IndexName: "", ReturnNum: insertSameNum + 1
      }];

   var actExplains = getMainclExplain( dbclPrimary, findConf );
   checkExplain( actExplains, expExplains );

   //var actExplains = getMainclExplain( dbclSlave, findConf ); 
   //checkExplain( actExplains, expExplains ); 

   println( "check maincl key after analyze subcs success!" );

   //子表分区键查询
   var findConf = { a0: { $in: [0, 10000] } };
   var expExplains = [
      {
         GroupName: srcGroupName, Name: subclFullName1,
         ScanType: "tbscan", IndexName: "", ReturnNum: insertSameNum + 1
      },
      {
         GroupName: srcGroupName, Name: subclFullName2,
         ScanType: "ixscan", IndexName: "$shard", ReturnNum: 0
      },
      {
         GroupName: srcGroupName, Name: subclFullName3,
         ScanType: "ixscan", IndexName: "$shard", ReturnNum: 0
      },
      {
         GroupName: srcGroupName, Name: subclFullName4,
         ScanType: "ixscan", IndexName: "$shard", ReturnNum: 0
      },
      {
         GroupName: desGroupName, Name: subclFullName2,
         ScanType: "ixscan", IndexName: "$shard", ReturnNum: 0
      },
      {
         GroupName: desGroupName, Name: subclFullName3,
         ScanType: "tbscan", IndexName: "", ReturnNum: insertSameNum + 1
      },
      {
         GroupName: desGroupName, Name: subclFullName4,
         ScanType: "ixscan", IndexName: "$shard", ReturnNum: 0
      }];

   var actExplains = getMainclExplain( dbclPrimary, findConf );
   checkExplain( actExplains, expExplains );

   //var actExplains = getMainclExplain( dbclSlave, findConf ); 
   //checkExplain( actExplains, expExplains ); 

   println( "check subcl key after analyze subcs success!" );

   //索引键查询
   var findConf = { a1: { $in: [0, 10000] } };
   var expExplains = [
      {
         GroupName: srcGroupName, Name: subclFullName1,
         ScanType: "tbscan", IndexName: "", ReturnNum: insertSameNum + 1
      },
      {
         GroupName: desGroupName, Name: subclFullName1,
         ScanType: "ixscan", IndexName: "a1", ReturnNum: 0
      },
      {
         GroupName: srcGroupName, Name: subclFullName2,
         ScanType: "ixscan", IndexName: "a1", ReturnNum: 0
      },
      {
         GroupName: desGroupName, Name: subclFullName2,
         ScanType: "ixscan", IndexName: "a1", ReturnNum: 0
      },
      {
         GroupName: srcGroupName, Name: subclFullName3,
         ScanType: "ixscan", IndexName: "a1", ReturnNum: 0
      },
      {
         GroupName: desGroupName, Name: subclFullName3,
         ScanType: "tbscan", IndexName: "", ReturnNum: insertSameNum + 1
      },
      {
         GroupName: srcGroupName, Name: subclFullName4,
         ScanType: "ixscan", IndexName: "a1", ReturnNum: 0
      },
      {
         GroupName: desGroupName, Name: subclFullName4,
         ScanType: "ixscan", IndexName: "a1", ReturnNum: 0
      }];

   var actExplains = getMainclExplain( dbclPrimary, findConf );
   checkExplain( actExplains, expExplains );

   //var actExplains = getMainclExplain( dbclSlave, findConf ); 
   //checkExplain( actExplains, expExplains ); 

   println( "check index in maincs after analyze subcs success!" );
}
