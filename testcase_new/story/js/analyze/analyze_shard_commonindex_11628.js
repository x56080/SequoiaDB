/************************************
*@Description:   指定shard索引生成默认统计信息并修手工改统计信息再清空
*@author:      liuxiaoxuan
*@createdate:  2017.11.13
*@testlinkCase: seqDB-11628
**************************************/
function main ()
{
   if( commIsStandalone( db ) )
   {
      println( "skip standalone environment" );
      return;
   }

   if( 2 > commGetGroupsNum( db ) )
   {
      println( "group less than 2" );
      return;
   }

   var csName = COMMCSNAME + "11628";
   commDropCS( db, csName, true, "drop CS in the beginning" );

   commCreateCS( db, csName, false, "" );

   //create cl
   var clOption = { ShardingKey: { a: 1 }, ShardingType: "hash" };
   var clName = COMMCLNAME + "11628";
   var dbcl = commCreateCL( db, csName, clName, clOption, true );

   //get master/slave datanode
   var db1 = new Sdb( db );
   db1.setSessionAttr( { PreferedInstance: "m" } );
   var dbclPrimary = db1.getCS( csName ).getCL( clName );

   //   db1 = new Sdb( db ); 
   //   db1.setSessionAttr( {PreferedInstance: "s"} ); 
   //   var dbclSlave = db1.getCS( csName ).getCL( clName ); 

   //insert datas
   var insertNums = 10000;
   var sameValues = 90002;
   insertDiffDatas( dbcl, insertNums );
   insertSameDatas( dbcl, insertNums, sameValues );

   //create index
   commCreateIndex( dbcl, "b", { b: 1 }, false );

   //split cl
   var groups = ClSplitOneTimes( csName, clName, 50 );

   //analyze
   var cl_full_name = csName + "." + clName;
   var options = { Collection: cl_full_name };
   analyze( db, options );

   //check before analyze success
   checkConsistency( db, csName, clName );
   checkStat( db, csName, clName, "$shard", true, true );
   checkStat( db, csName, clName, "b", true, true );

   //check out snapshot access plans
   var accessFindOption = { Collection: cl_full_name };
   var actAccessPlans = getSplitAccessPlans( db, accessFindOption );
   var expAccessPlans = [];
   checkSnapShotAccessPlans( cl_full_name, expAccessPlans, actAccessPlans );

   //get split groupName
   var srcGroupName = groups[0].GroupName;
   var destGroupName = groups[1].GroupName;

   //check the query explain before analyze
   var findConf1 = { a: sameValues };
   var findConf2 = { b: sameValues };

   var expExplains1 = [
      {
         ScanType: "tbscan", IndexName: "",
         GroupName: srcGroupName, ReturnNum: insertNums
      }];
   var expExplains2 = [{ ScanType: "tbscan", IndexName: "", GroupName: srcGroupName, ReturnNum: insertNums },
   { ScanType: "ixscan", IndexName: "b", GroupName: destGroupName, ReturnNum: 0 }];

   var actExplains1 = getSplitExplain( dbclPrimary, findConf1 );
   var actExplains2 = getSplitExplain( dbclPrimary, findConf2 );
   checkExplain( actExplains1, expExplains1 );
   checkExplain( actExplains2, expExplains2 );

   //   var actExplains1 = getSplitExplain( dbclSlave, findConf1 ); 
   //   var actExplains2 = getSplitExplain( dbclSlave, findConf2 ); 
   //   checkExplain( actExplains1, expExplains1 ); 
   //   checkExplain( actExplains2, expExplains2 ); 

   //query
   query( dbclPrimary, findConf1, null, null, insertNums );
   query( dbclPrimary, findConf2, null, null, insertNums );
   //   query( dbclSlave, findConf1, null, null, insertNums ); 
   //   query( dbclSlave, findConf2, null, null, insertNums ); 

   //check out snapshot access plans
   var accessFindOption = { Collection: cl_full_name };

   var actAccessPlans = getSplitAccessPlans( db, accessFindOption );
   var expAccessPlans = [{ ScanType: "tbscan", IndexName: "", GroupName: srcGroupName },
   { ScanType: "tbscan", IndexName: "", GroupName: srcGroupName },
   { ScanType: "ixscan", IndexName: "b", GroupName: destGroupName }];

   checkSnapShotAccessPlans( cl_full_name, expAccessPlans, actAccessPlans );

   println( "check result before default analyze !" );

   //analyze with index
   var options1 = { Mode: 3, Collection: cl_full_name, Index: "$shard" };
   analyze( db, options1 );
   var options2 = { Mode: 3, Collection: cl_full_name, Index: "b" };
   analyze( db, options2 );

   //check after analyze with shard index
   checkConsistency( db, csName, clName );
   checkStat( db, csName, clName, "$shard", true, false );
   checkStat( db, csName, clName, "b", true, false );

   //check out snapshot access plans
   var accessFindOption = { Collection: cl_full_name };
   var actAccessPlans = getSplitAccessPlans( db, accessFindOption );
   var expAccessPlans = [];
   checkSnapShotAccessPlans( cl_full_name, expAccessPlans, actAccessPlans );

   var expExplains1 = [
      {
         ScanType: "ixscan", IndexName: "$shard",
         GroupName: srcGroupName, ReturnNum: insertNums
      }];
   var expExplains2 = [{ ScanType: "ixscan", IndexName: "b", GroupName: srcGroupName, ReturnNum: insertNums },
   { ScanType: "ixscan", IndexName: "b", GroupName: destGroupName, ReturnNum: 0 }];

   var actExplains1 = getSplitExplain( dbclPrimary, findConf1 );
   var actExplains2 = getSplitExplain( dbclPrimary, findConf2 );
   checkExplain( actExplains1, expExplains1 );
   checkExplain( actExplains2, expExplains2 );

   //   var actExplains1 = getSplitExplain( dbclSlave, findConf1 ); 
   //   var actExplains2 = getSplitExplain( dbclSlave, findConf2 ); 
   //   checkExplain( actExplains1, expExplains1 ); 
   //   checkExplain( actExplains2, expExplains2 ); 

   //query
   query( dbclPrimary, findConf1, null, null, insertNums );
   query( dbclPrimary, findConf2, null, null, insertNums );
   //   query( dbclSlave, findConf1, null, null, insertNums ); 
   //   query( dbclSlave, findConf2, null, null, insertNums ); 

   //check out snapshot access plans
   var accessFindOption = { Collection: cl_full_name };

   var actAccessPlans = getSplitAccessPlans( db, accessFindOption );
   var expAccessPlans = [{ ScanType: "ixscan", IndexName: "$shard", GroupName: srcGroupName },
   { ScanType: "ixscan", IndexName: "b", GroupName: srcGroupName },
   { ScanType: "ixscan", IndexName: "b", GroupName: destGroupName }];

   checkSnapShotAccessPlans( cl_full_name, expAccessPlans, actAccessPlans );

   println( "check result after default analyze with index !" );

   // modify SYSSTAT info
   var mcvValues1 = [{ a: 0 }, { a: 1 }, { a: sameValues }];
   var fracs1 = [500, 500, 9000];
   updateIndexStateInfo( db, csName, clName, "$shard", mcvValues1, fracs1 );

   var mcvValues2 = [{ b: 0 }, { b: 1 }, { b: sameValues }];
   var fracs2 = [500, 500, 9000];
   updateIndexStateInfo( db, csName, clName, "b", mcvValues2, fracs2 );

   // reload analyze
   var options1 = { Mode: 4, Collection: cl_full_name, Index: "$shard" };
   analyze( db, options1 );
   var options2 = { Mode: 4, Collection: cl_full_name, Index: "b" };
   analyze( db, options2 );

   checkConsistency( db, csName, clName );
   checkStat( db, csName, clName, "$shard", true, true );
   checkStat( db, csName, clName, "b", true, true );

   //check out snapshot access plans
   var accessFindOption = { Collection: cl_full_name };
   var actAccessPlans = getSplitAccessPlans( db, accessFindOption );
   var expAccessPlans = [];
   checkSnapShotAccessPlans( cl_full_name, expAccessPlans, actAccessPlans );

   var expExplains1 = [
      {
         ScanType: "tbscan", IndexName: "",
         GroupName: srcGroupName, ReturnNum: insertNums
      }];
   var expExplains2 = [{ ScanType: "tbscan", IndexName: "", GroupName: srcGroupName, ReturnNum: insertNums },
   { ScanType: "tbscan", IndexName: "", GroupName: destGroupName, ReturnNum: 0 }];

   var actExplains1 = getSplitExplain( dbclPrimary, findConf1 );
   var actExplains2 = getSplitExplain( dbclPrimary, findConf2 );
   checkExplain( actExplains1, expExplains1 );
   checkExplain( actExplains2, expExplains2 );

   //   var actExplains1 = getSplitExplain( dbclSlave, findConf1 ); 
   //   var actExplains2 = getSplitExplain( dbclSlave, findConf2 ); 
   //   checkExplain( actExplains1, expExplains1 ); 
   //   checkExplain( actExplains2, expExplains2 ); 

   //query
   query( dbclPrimary, findConf1, null, null, insertNums );
   query( dbclPrimary, findConf2, null, null, insertNums );
   //   query( dbclSlave, findConf1, null, null, insertNums ); 
   //   query( dbclSlave, findConf2, null, null, insertNums ); 

   //check out snapshot access plans
   var accessFindOption = { Collection: cl_full_name };

   var actAccessPlans = getSplitAccessPlans( db, accessFindOption );
   var expAccessPlans = [{ ScanType: "tbscan", IndexName: "", GroupName: srcGroupName },
   { ScanType: "tbscan", IndexName: "", GroupName: srcGroupName },
   { ScanType: "tbscan", IndexName: "", GroupName: destGroupName }];

   checkSnapShotAccessPlans( cl_full_name, expAccessPlans, actAccessPlans );

   println( "check result after reload analyze!" );

   //truncate invalidate
   var options1 = { Mode: 5, Collection: cl_full_name, Index: "$shard" };
   analyze( db, options1 );
   var options2 = { Mode: 5, Collection: cl_full_name, Index: "b" };
   analyze( db, options2 );

   checkConsistency( db, csName, clName );
   checkStat( db, csName, clName, "$shard", true, true );
   checkStat( db, csName, clName, "b", true, true );

   //check out snapshot access plans
   var accessFindOption = { Collection: cl_full_name };
   var actAccessPlans = getSplitAccessPlans( db, accessFindOption );
   var expAccessPlans = [];
   checkSnapShotAccessPlans( cl_full_name, expAccessPlans, actAccessPlans );

   var expExplains1 = [
      {
         ScanType: "tbscan", IndexName: "",
         GroupName: srcGroupName, ReturnNum: insertNums
      }];
   var expExplains2 = [{ ScanType: "tbscan", IndexName: "", GroupName: srcGroupName, ReturnNum: insertNums },
   { ScanType: "tbscan", IndexName: "", GroupName: destGroupName, ReturnNum: 0 }];

   var actExplains1 = getSplitExplain( dbclPrimary, findConf1 );
   var actExplains2 = getSplitExplain( dbclPrimary, findConf2 );
   checkExplain( actExplains1, expExplains1 );
   checkExplain( actExplains2, expExplains2 );

   //   var actExplains1 = getSplitExplain( dbclSlave, findConf1 ); 
   //   var actExplains2 = getSplitExplain( dbclSlave, findConf2 ); 
   //   checkExplain( actExplains1, expExplains1 ); 
   //   checkExplain( actExplains2, expExplains2 ); 

   //query
   query( dbclPrimary, findConf1, null, null, insertNums );
   query( dbclPrimary, findConf2, null, null, insertNums );
   //   query( dbclSlave, findConf1, null, null, insertNums ); 
   //   query( dbclSlave, findConf2, null, null, insertNums ); 

   //check out snapshot access plans
   var accessFindOption = { Collection: cl_full_name };

   var actAccessPlans = getSplitAccessPlans( db, accessFindOption );
   var expAccessPlans = [{ ScanType: "tbscan", IndexName: "", GroupName: srcGroupName },
   { ScanType: "tbscan", IndexName: "", GroupName: srcGroupName },
   { ScanType: "tbscan", IndexName: "", GroupName: destGroupName }];

   checkSnapShotAccessPlans( cl_full_name, expAccessPlans, actAccessPlans );

   println( "check result after truncate invalidate!" );

   // modify SYSSTAT info again
   var mcvValues1 = [{ a: 0 }, { a: 1 }, { a: sameValues }];
   var fracs1 = [500, 500, 500];
   updateIndexStateInfo( db, csName, clName, "$shard", mcvValues1, fracs1 );

   var mcvValues2 = [{ b: 0 }, { b: 1 }, { b: sameValues }];
   var fracs2 = [500, 500, 500];
   updateIndexStateInfo( db, csName, clName, "b", mcvValues2, fracs2 );

   var expExplains1 = [
      {
         ScanType: "tbscan", IndexName: "",
         GroupName: srcGroupName, ReturnNum: insertNums
      }];
   var expExplains2 = [{ ScanType: "tbscan", IndexName: "", GroupName: srcGroupName, ReturnNum: insertNums },
   { ScanType: "tbscan", IndexName: "", GroupName: destGroupName, ReturnNum: 0 }];

   var actExplains1 = getSplitExplain( dbclPrimary, findConf1 );
   var actExplains2 = getSplitExplain( dbclPrimary, findConf2 );
   checkExplain( actExplains1, expExplains1 );
   checkExplain( actExplains2, expExplains2 );

   //   var actExplains1 = getSplitExplain( dbclSlave, findConf1 ); 
   //   var actExplains2 = getSplitExplain( dbclSlave, findConf2 ); 
   //   checkExplain( actExplains1, expExplains1 ); 
   //   checkExplain( actExplains2, expExplains2 ); 

   //query
   query( dbclPrimary, findConf1, null, null, insertNums );
   query( dbclPrimary, findConf2, null, null, insertNums );
   //   query( dbclSlave, findConf1, null, null, insertNums ); 
   //   query( dbclSlave, findConf2, null, null, insertNums ); 

   //check out snapshot access plans
   var accessFindOption = { Collection: cl_full_name };

   var actAccessPlans = getSplitAccessPlans( db, accessFindOption );
   var expAccessPlans = [{ ScanType: "tbscan", IndexName: "", GroupName: srcGroupName },
   { ScanType: "tbscan", IndexName: "", GroupName: srcGroupName },
   { ScanType: "tbscan", IndexName: "", GroupName: destGroupName }];

   checkSnapShotAccessPlans( cl_full_name, expAccessPlans, actAccessPlans );

   println( "check result after modify SYS info but without reload analyze!" );

   //truncate invalidate
   var options = { Mode: 5, Collection: cl_full_name };
   analyze( db, options );

   checkConsistency( db, csName, clName );
   checkStat( db, csName, clName, "$shard", true, true );
   checkStat( db, csName, clName, "b", true, true );

   //check out snapshot access plans
   var accessFindOption = { Collection: cl_full_name };
   var actAccessPlans = getSplitAccessPlans( db, accessFindOption );
   var expAccessPlans = [];
   checkSnapShotAccessPlans( cl_full_name, expAccessPlans, actAccessPlans );

   var expExplains1 = [
      {
         ScanType: "ixscan", IndexName: "$shard",
         GroupName: srcGroupName, ReturnNum: insertNums
      }];
   var expExplains2 = [{ ScanType: "ixscan", IndexName: "b", GroupName: srcGroupName, ReturnNum: insertNums },
   { ScanType: "ixscan", IndexName: "b", GroupName: destGroupName, ReturnNum: 0 }];

   var actExplains1 = getSplitExplain( dbclPrimary, findConf1 );
   var actExplains2 = getSplitExplain( dbclPrimary, findConf2 );
   checkExplain( actExplains1, expExplains1 );
   checkExplain( actExplains2, expExplains2 );

   //   var actExplains1 = getSplitExplain( dbclSlave, findConf1 ); 
   //   var actExplains2 = getSplitExplain( dbclSlave, findConf2 ); 
   //   checkExplain( actExplains1, expExplains1 ); 
   //   checkExplain( actExplains2, expExplains2 ); 

   //query
   query( dbclPrimary, findConf1, null, null, insertNums );
   query( dbclPrimary, findConf2, null, null, insertNums );
   //   query( dbclSlave, findConf1, null, null, insertNums ); 
   //   query( dbclSlave, findConf2, null, null, insertNums ); 

   //check out snapshot access plans
   var accessFindOption = { Collection: cl_full_name };

   var actAccessPlans = getSplitAccessPlans( db, accessFindOption );
   var expAccessPlans = [{ ScanType: "ixscan", IndexName: "$shard", GroupName: srcGroupName },
   { ScanType: "ixscan", IndexName: "b", GroupName: srcGroupName },
   { ScanType: "ixscan", IndexName: "b", GroupName: destGroupName }];

   checkSnapShotAccessPlans( cl_full_name, expAccessPlans, actAccessPlans );

   println( "check result after truncate invalidate again!" );

   db1.close();
   commDropCS( db, csName, true, "drop CS in the end" );
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

