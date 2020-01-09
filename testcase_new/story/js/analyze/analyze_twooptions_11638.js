/************************************
*@Description: seqDB-11638:指定2个参数组合生成默认统计信息并手工修改再清空
*@author:      liuxiaoxuan
*@createdate:  2017.11.13
**************************************/
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

   var csName = COMMCSNAME + "11638";
   commDropCS( db, csName, true, "drop CS in the beginning" );

   commCreateCS( db, csName, false, "" );

   //create cl
   var clName = COMMCLNAME + "11638";
   var dbcl = commCreateCL( db, csName, clName );

   var clFullName = csName + "." + clName;

   //get master/slave datanode
   var db1 = new Sdb( db );
   db1.setSessionAttr( { PreferedInstance: "m" } );
   var dbclPrimary = db1.getCS( csName ).getCL( clName );

   //   db1 = new Sdb( db ); 
   //   db1.setSessionAttr( {PreferedInstance: "s"} ); 
   //   var dbclSlave = db1.getCS( csName ).getCL( clName ); 

   //insert datas
   var insertNums = 3000;
   var sameValues = 9000;

   insertDiffDatas( dbcl, insertNums );
   insertSameDatas( dbcl, insertNums, sameValues );

   //create index
   commCreateIndex( dbcl, "a", { a: 1 }, false );

   //analyze
   var cl_full_name = csName + "." + clName;
   var options = { Collection: cl_full_name };
   analyze( db, options );

   //check analyze
   checkConsistency( db, csName, clName );
   checkStat( db, csName, clName, "a", true, true );

   //check out snapshot access plans
   var accessFindOption = { Collection: clFullName };
   var actAccessPlans = getCommonAccessPlans( db, accessFindOption );
   var expAccessPlans = [];
   checkSnapShotAccessPlans( clFullName, expAccessPlans, actAccessPlans );

   var findConf = { a: 9000 };
   var expExplains = [{ ScanType: "tbscan", IndexName: "", ReturnNum: insertNums }];

   var actExplains = getCommonExplain( dbclPrimary, findConf );
   checkExplain( actExplains, expExplains );

   //   var actExplains = getCommonExplain( dbclSlave, findConf ); 
   //   checkExplain( actExplains, expExplains ); 

   //query
   query( dbclPrimary, findConf, null, null, insertNums );
   //   query( dbclSlave, findConf, null, null, insertNums ); 

   //check out snapshot access plans
   var accessFindOption = { Collection: clFullName };

   var actAccessPlans = getCommonAccessPlans( db, accessFindOption );
   var expAccessPlans = [{ ScanType: "tbscan", IndexName: "" }];

   checkSnapShotAccessPlans( clFullName, expAccessPlans, actAccessPlans );

   //get Group and Node info
   var groupName = commGetCLGroups( db, csName + "." + clName );
   var groupDetail = commGetGroups( db, false, groupName );

   var groupId = groupDetail[0][0].GroupID;
   var priNodeId = groupDetail[0][0].PrimaryNode;

   //generate default analyze info with cl + group
   var options = { Mode: 3, Collection: cl_full_name, GroupName: groupName };
   analyze( db, options );

   //check after default analyze success with cl + group
   checkConsistency( db, csName, clName );
   checkStat( db, csName, clName, "a", true, false );

   //check out snapshot access plans
   var accessFindOption = { Collection: clFullName };
   var actAccessPlans = getCommonAccessPlans( db, accessFindOption );
   var expAccessPlans = [];
   checkSnapShotAccessPlans( clFullName, expAccessPlans, actAccessPlans );

   var findConf = { a: 9000 };
   var expExplains = [{ ScanType: "ixscan", IndexName: "a", ReturnNum: insertNums }];

   var actExplains = getCommonExplain( dbclPrimary, findConf );
   checkExplain( actExplains, expExplains );

   //   var actExplains = getCommonExplain( dbclSlave, findConf ); 
   //   checkExplain( actExplains, expExplains ); 

   //query
   query( dbclPrimary, findConf, null, null, insertNums );
   //   query( dbclSlave, findConf, null, null, insertNums ); 

   //check out snapshot access plans
   var accessFindOption = { Collection: clFullName };

   var actAccessPlans = getCommonAccessPlans( db, accessFindOption );
   var expAccessPlans = [{ ScanType: "ixscan", IndexName: "a" }];

   checkSnapShotAccessPlans( clFullName, expAccessPlans, actAccessPlans );

   println( "check result after default analyze with cl+group!" );

   //modify and reload analyze with cl + group
   var mcvValues = [{ a: 0 }, { a: 1 }, { a: 9000 }];
   var fracs = [500, 500, 9000];
   updateIndexStateInfo( db, csName, clName, "a", mcvValues, fracs );

   var options = { Mode: 4, Collection: cl_full_name, GroupName: groupName };
   analyze( db, options );

   //check after reload analyze with cl + group
   checkConsistency( db, csName, clName );
   checkStat( db, csName, clName, "a", true, true );

   //check out snapshot access plans
   var accessFindOption = { Collection: clFullName };
   var actAccessPlans = getCommonAccessPlans( db, accessFindOption );
   var expAccessPlans = [];
   checkSnapShotAccessPlans( clFullName, expAccessPlans, actAccessPlans );

   //check the query explain after analyze
   var findConf = { a: 9000 };
   var expExplains = [{ ScanType: "tbscan", IndexName: "", ReturnNum: insertNums }];

   var actExplains = getCommonExplain( dbclPrimary, findConf );
   checkExplain( actExplains, expExplains );

   //   var actExplains = getCommonExplain( dbclSlave, findConf ); 
   //   checkExplain( actExplains, expExplains ); 

   //query
   query( dbclPrimary, findConf, null, null, insertNums );
   //   query( dbclSlave, findConf, null, null, insertNums ); 

   //check out snapshot access plans
   var accessFindOption = { Collection: clFullName };

   var actAccessPlans = getCommonAccessPlans( db, accessFindOption );
   var expAccessPlans = [{ ScanType: "tbscan", IndexName: "" }];
   checkSnapShotAccessPlans( clFullName, expAccessPlans, actAccessPlans );

   println( "check result after reload analyze with cl+group!" );

   //truncate invalidate with cl + group
   var options = { Mode: 5, Collection: cl_full_name, GroupName: groupName };
   analyze( db, options );

   //check analyze after truncate invalidate with cl + group
   checkConsistency( db, csName, clName );
   checkStat( db, csName, clName, "a", true, true );

   //check out snapshot access plans
   var accessFindOption = { Collection: clFullName };
   var actAccessPlans = getCommonAccessPlans( db, accessFindOption );
   var expAccessPlans = [];
   checkSnapShotAccessPlans( clFullName, expAccessPlans, actAccessPlans );

   var findConf = { a: 9000 };
   var expExplains = [{ ScanType: "tbscan", IndexName: "", ReturnNum: insertNums }];

   var actExplains = getCommonExplain( dbclPrimary, findConf );
   checkExplain( actExplains, expExplains );

   //   var actExplains = getCommonExplain( dbclSlave, findConf ); 
   //   checkExplain( actExplains, expExplains ); 

   //query
   query( dbclPrimary, findConf, null, null, insertNums );
   //   query( dbclSlave, findConf, null, null, insertNums ); 

   //check out snapshot access plans
   var accessFindOption = { Collection: clFullName };

   var actAccessPlans = getCommonAccessPlans( db, accessFindOption );
   var expAccessPlans = [{ ScanType: "tbscan", IndexName: "" }];

   checkSnapShotAccessPlans( clFullName, expAccessPlans, actAccessPlans );

   println( "check result after truncate invalidate with cl+group!" );

   //modify SYS info again
   var mcvValues = [{ a: 0 }, { a: 1 }, { a: 9000 }];
   var fracs = [500, 500, 500];
   updateIndexStateInfo( db, csName, clName, "a", mcvValues, fracs );

   checkConsistency( db, csName, clName );
   checkStat( db, csName, clName, "a", true, true );

   var findConf = { a: 9000 };
   var expExplains = [{ ScanType: "tbscan", IndexName: "", ReturnNum: insertNums }];

   var actExplains = getCommonExplain( dbclPrimary, findConf );
   checkExplain( actExplains, expExplains );

   //   var actExplains = getCommonExplain( dbclSlave, findConf ); 
   //   checkExplain( actExplains, expExplains ); 

   //query
   query( dbclPrimary, findConf, null, null, insertNums );
   //   query( dbclSlave, findConf, null, null, insertNums ); 

   //check out snapshot access plans
   var accessFindOption = { Collection: clFullName };

   var actAccessPlans = getCommonAccessPlans( db, accessFindOption );
   var expAccessPlans = [{ ScanType: "tbscan", IndexName: "" }];
   checkSnapShotAccessPlans( clFullName, expAccessPlans, actAccessPlans );
   println( "check result modify SYS info without analyze!" );

   //truncate invalidate again with cs + group
   var options = { Mode: 5, CollectionSpace: csName, GroupName: groupName };
   analyze( db, options );

   //check analyze after truncate invalidate with cs + group
   checkConsistency( db, csName, clName );
   checkStat( db, csName, clName, "a", true, true );

   //check out snapshot access plans
   var accessFindOption = { Collection: clFullName };
   var actAccessPlans = getCommonAccessPlans( db, accessFindOption );
   var expAccessPlans = [];
   checkSnapShotAccessPlans( clFullName, expAccessPlans, actAccessPlans );

   var findConf = { a: 9000 };
   var expExplains = [{ ScanType: "ixscan", IndexName: "a", ReturnNum: insertNums }];

   var actExplains = getCommonExplain( dbclPrimary, findConf );
   checkExplain( actExplains, expExplains );

   //   var actExplains = getCommonExplain( dbclSlave, findConf ); 
   //   checkExplain( actExplains, expExplains ); 

   //query
   query( dbclPrimary, findConf, null, null, insertNums );
   //   query( dbclSlave, findConf, null, null, insertNums ); 

   //check out snapshot access plans
   var accessFindOption = { Collection: clFullName };

   var actAccessPlans = getCommonAccessPlans( db, accessFindOption );
   var expAccessPlans = [{ ScanType: "ixscan", IndexName: "a" }];
   checkSnapShotAccessPlans( clFullName, expAccessPlans, actAccessPlans );
   println( "check truncate invalidate end with cs+group!" );

   //generate default analyze info with cl + node
   var options = { Mode: 3, Collection: cl_full_name, NodeID: priNodeId };
   analyze( db, options );

   //check after default analyze success with cl + node
   checkConsistency( db, csName, clName );
   checkStat( db, csName, clName, "a", true, false );

   //check out snapshot access plans
   var accessFindOption = { Collection: clFullName };
   var actAccessPlans = getCommonAccessPlans( db, accessFindOption );
   var expAccessPlans = [];
   checkSnapShotAccessPlans( clFullName, expAccessPlans, actAccessPlans );

   var findConf = { a: 9000 };
   var expExplains = [{ ScanType: "ixscan", IndexName: "a", ReturnNum: insertNums }];

   var actExplains = getCommonExplain( dbclPrimary, findConf );
   checkExplain( actExplains, expExplains );

   //   var actExplains = getCommonExplain( dbclSlave, findConf ); 
   //   checkExplain( actExplains, expExplains ); 

   //query
   query( dbclPrimary, findConf, null, null, insertNums );
   //   query( dbclSlave, findConf, null, null, insertNums ); 

   //check out snapshot access plans
   var accessFindOption = { Collection: clFullName };

   var actAccessPlans = getCommonAccessPlans( db, accessFindOption );
   var expAccessPlans = [{ ScanType: "ixscan", IndexName: "a" }];

   checkSnapShotAccessPlans( clFullName, expAccessPlans, actAccessPlans );

   println( "check result after default analyze with cl+node!" );

   //modify and reload analyze with cl + node
   var mcvValues = [{ a: 0 }, { a: 1 }, { a: 9000 }];
   var fracs = [500, 500, 9000];
   updateIndexStateInfo( db, csName, clName, "a", mcvValues, fracs );

   var options = { Mode: 4, Collection: cl_full_name, NodeID: priNodeId };
   analyze( db, options );

   //check after reload analyze with cl + node
   checkConsistency( db, csName, clName );
   checkStat( db, csName, clName, "a", true, true );

   //check out snapshot access plans
   var accessFindOption = { Collection: clFullName };
   var actAccessPlans = getCommonAccessPlans( db, accessFindOption );
   var expAccessPlans = [];
   checkSnapShotAccessPlans( clFullName, expAccessPlans, actAccessPlans );

   //check the query explain after analyze
   var findConf = { a: 9000 };
   var expExplains1 = [{ ScanType: "tbscan", IndexName: "", ReturnNum: insertNums }];
   var expExplains2 = [{ ScanType: "ixscan", IndexName: "a", ReturnNum: insertNums }];

   var actExplains = getCommonExplain( dbclPrimary, findConf );
   checkExplain( actExplains, expExplains1 );

   //   var actExplains = getCommonExplain( dbclSlave, findConf ); 
   //   checkExplain( actExplains, expExplains2 ); 

   //query
   query( dbclPrimary, findConf, null, null, insertNums );
   //   query( dbclSlave, findConf, null, null, insertNums ); 

   //check out snapshot access plans
   var accessFindOption = { Collection: clFullName };

   var actAccessPlans = getCommonAccessPlans( db, accessFindOption );
   var expAccessPlans = [{ ScanType: "tbscan", IndexName: "" }];
   checkSnapShotAccessPlans( clFullName, expAccessPlans, actAccessPlans );
   println( "check result after reload analyze with cl+node!" );

   //truncate invalidate with cl + node
   var options = { Mode: 5, Collection: cl_full_name, NodeID: priNodeId };
   analyze( db, options );

   //check analyze after truncate invalidate
   checkConsistency( db, csName, clName );
   checkStat( db, csName, clName, "a", true, true );

   //check out snapshot access plans
   var accessFindOption = { Collection: clFullName };
   var actAccessPlans = getCommonAccessPlans( db, accessFindOption );
   var expAccessPlans = [];
   checkSnapShotAccessPlans( clFullName, expAccessPlans, actAccessPlans );

   var findConf = { a: 9000 };
   var expExplains1 = [{ ScanType: "tbscan", IndexName: "", ReturnNum: insertNums }];
   var expExplains2 = [{ ScanType: "ixscan", IndexName: "a", ReturnNum: insertNums }];

   var actExplains = getCommonExplain( dbclPrimary, findConf );
   checkExplain( actExplains, expExplains1 );

   //   var actExplains = getCommonExplain( dbclSlave, findConf ); 
   //   checkExplain( actExplains, expExplains2 ); 

   //query
   query( dbclPrimary, findConf, null, null, insertNums );
   //   query( dbclSlave, findConf, null, null, insertNums ); 

   //check out snapshot access plans
   var accessFindOption = { Collection: clFullName };

   var actAccessPlans = getCommonAccessPlans( db, accessFindOption );
   var expAccessPlans = [{ ScanType: "tbscan", IndexName: "" }];
   checkSnapShotAccessPlans( clFullName, expAccessPlans, actAccessPlans );

   println( "check result after truncate invalidate with cl+node!" );

   //modify SYS info again
   var mcvValues = [{ a: 0 }, { a: 1 }, { a: 9000 }];
   var fracs = [500, 500, 500];
   updateIndexStateInfo( db, csName, clName, "a", mcvValues, fracs );

   checkConsistency( db, csName, clName );
   checkStat( db, csName, clName, "a", true, true );

   var findConf = { a: 9000 };
   var expExplains1 = [{ ScanType: "tbscan", IndexName: "", ReturnNum: insertNums }];
   var expExplains2 = [{ ScanType: "ixscan", IndexName: "a", ReturnNum: insertNums }];

   var actExplains = getCommonExplain( dbclPrimary, findConf );
   checkExplain( actExplains, expExplains1 );

   //   var actExplains = getCommonExplain( dbclSlave, findConf ); 
   //   checkExplain( actExplains, expExplains2 ); 

   //query                                                 //query
   query( dbclPrimary, findConf, null, null, insertNums );
   //   query( dbclSlave, findConf, null, null, insertNums ); 

   //check out snapshot access plans
   var accessFindOption = { Collection: clFullName };

   var actAccessPlans = getCommonAccessPlans( db, accessFindOption );
   var expAccessPlans = [{ ScanType: "tbscan", IndexName: "" }];
   checkSnapShotAccessPlans( clFullName, expAccessPlans, actAccessPlans );

   println( "check result modify SYS info without analyze!" );

   //truncate invalidate again with cs + node
   var options = { Mode: 5, CollectionSpace: csName, NodeID: priNodeId };
   analyze( db, options );

   //check analyze after truncate invalidate with cs + node
   checkConsistency( db, csName, clName );
   checkStat( db, csName, clName, "a", true, true );

   //check out snapshot access plans
   var accessFindOption = { Collection: clFullName };
   var actAccessPlans = getCommonAccessPlans( db, accessFindOption );
   var expAccessPlans = [];
   checkSnapShotAccessPlans( clFullName, expAccessPlans, actAccessPlans );

   var findConf = { a: 9000 };
   var expExplains = [{ ScanType: "ixscan", IndexName: "a", ReturnNum: insertNums }];

   var actExplains = getCommonExplain( dbclPrimary, findConf );
   checkExplain( actExplains, expExplains );

   //   var actExplains = getCommonExplain( dbclSlave, findConf ); 
   //   checkExplain( actExplains, expExplains ); 

   //query
   query( dbclPrimary, findConf, null, null, insertNums );
   //   query( dbclSlave, findConf, null, null, insertNums ); 

   //check out snapshot access plans
   var accessFindOption = { Collection: clFullName };

   var actAccessPlans = getCommonAccessPlans( db, accessFindOption );
   var expAccessPlans = [{ ScanType: "ixscan", IndexName: "a" }];

   checkSnapShotAccessPlans( clFullName, expAccessPlans, actAccessPlans );

   println( "check truncate invalidate end with cs+node!" );

   //check invalid analyze, cs + cl, cs + index, index + group, index + node
   var options = [{ Mode: 3, CollectionSpace: csName, Collection: cl_full_name },
   { Mode: 3, CollectionSpace: csName, GroupName: groupName },
   { Mode: 3, CollectionSpace: csName, Index: "a" },
   { Mode: 3, CollectionSpace: csName, NodeID: priNodeId },
   { Mode: 3, Index: "a", GroupName: groupName },
   { Mode: 3, Index: "a", NodeID: priNodeId },
   { Mode: 3, GroupID: groupId, NodeID: priNodeId },
   { Mode: 4, CollectionSpace: csName, Collection: cl_full_name },
   { Mode: 4, CollectionSpace: csName, Index: "a" },
   { Mode: 4, Index: "a", GroupName: groupName },
   { Mode: 4, Index: "a", NodeID: priNodeId },
   { Mode: 5, CollectionSpace: csName, Collection: cl_full_name },
   { Mode: 5, CollectionSpace: csName, Index: "a" },
   { Mode: 5, Index: "a", GroupName: groupName },
   { Mode: 5, Index: "a", NodeID: priNodeId }];

   for( var i in options )
   {
      checkAnalyzeInvalidResult( options[i] );
   }

   println( "check invalid analyze success!" );

   //check out snapshot access plans
   var accessFindOption = { Collection: clFullName };
   var actAccessPlans = getCommonAccessPlans( db, accessFindOption );
   var expAccessPlans = [{ ScanType: "ixscan", IndexName: "a" }];
   checkSnapShotAccessPlans( clFullName, expAccessPlans, actAccessPlans );

   db1.close();
   commDropCS( db, csName, true, "drop CS in the end" );
}

function checkAnalyzeInvalidResult ( options )
{
   try
   {
      db.analyze( options );
      throw new Error( "NEED ANALYZE FAILED" );
   }
   catch( e )
   {
      if( -6 != e.message )
      {
         throw e;
      }
   }
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
