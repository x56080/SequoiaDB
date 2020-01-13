/************************************
*@Description:  指定location其他参数收集统计信息   
*@author:      liuxiaoxuan
*@createdate:  2017.11.15
*@testlinkCase: seqDB-11759
**************************************/
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

function main ()
{
   if( commIsStandalone( db ) )
   {
      println( "skip standalone environment" );
      return;
   }

   var csName = COMMCSNAME + "11759";
   commDropCS( db, csName, true, "drop CS in the beginning" );

   commCreateCS( db, csName, false, "" );

   //create cl	
   var clName = COMMCLNAME + "11759";
   var dbcl = commCreateCL( db, csName, clName );

   var clFullName = csName + "." + clName;

   //insert datas
   var insertNums = 3000;
   var sameValues = 9000;

   insertDiffDatas( dbcl, insertNums );
   insertSameDatas( dbcl, insertNums, sameValues );

   //create index
   commCreateIndex( dbcl, "a", { a: 1 } );

   //get master/slave datanode
   var db1 = new Sdb( db );
   db1.setSessionAttr( { PreferedInstance: "m" } );
   var dbclPrimary = db1.getCS( csName ).getCL( clName );
   //   db1 = new Sdb(db);
   //   db1.setSessionAttr( {PreferedInstance: "s"} );
   //   var dbclSlave = db1.getCS(csName).getCL(clName);

   //check before analyze    
   checkConsistency( db, csName, clName );
   checkStat( db, csName, clName, "a", false, false );

   var findConf = { a: 9000 };
   var expExplains = [{ ScanType: "ixscan", IndexName: "a", ReturnNum: insertNums }];

   var actExplains = getCommonExplain( dbclPrimary, findConf );
   checkExplain( actExplains, expExplains );

   //   var actExplains = getCommonExplain( dbclSlave, findConf);
   //   checkExplain( actExplains, expExplains );

   //query
   query( dbclPrimary, findConf, null, null, insertNums );
   //   query(dbclSlave, findConf, null, null, insertNums);

   //check out snapshot access plans
   var accessFindOption = { Collection: clFullName };
   var actAccessPlans = getCommonAccessPlans( db, accessFindOption );
   var expAccessPlans = [{ ScanType: "ixscan", IndexName: "a" }];


   checkSnapShotAccessPlans( clFullName, expAccessPlans, actAccessPlans );

   println( "check result before correct analyze success!" );

   //get Group and Node info
   var groupName = commGetCLGroups( db, csName + "." + clName );
   var groupDetail = commGetGroups( db, false, groupName );

   var nodesInGroup = groupDetail[0];
   var groupId = nodesInGroup[0].GroupID;

   var primaryPos = nodesInGroup[0].PrimaryPos;
   var slavePos = parseInt( primaryPos % ( nodesInGroup.length - 1 ) ) + 1;

   var priNodeId = nodesInGroup[primaryPos].NodeID;
   var slaveNodeId = nodesInGroup[slavePos].NodeID;

   var priHostname = nodesInGroup[primaryPos].HostName;
   var priSvcname = nodesInGroup[primaryPos].svcname;

   var slaveHostname = nodesInGroup[slavePos].HostName;
   var slaveSvcname = nodesInGroup[slavePos].svcname;

   //analyze with group
   var options = { GroupID: groupId, GroupName: groupName };
   analyze( db, options );

   //check after correct analyze  
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

   //   var actExplains = getCommonExplain( dbclSlave, findConf);
   //   checkExplain( actExplains, expExplains );

   //query
   query( dbclPrimary, findConf, null, null, insertNums );
   //   query(dbclSlave, findConf, null, null, insertNums);

   //check out snapshot access plans
   var accessFindOption = { Collection: clFullName };
   var actAccessPlans = getCommonAccessPlans( db, accessFindOption );
   var expAccessPlans = [{ ScanType: "tbscan", IndexName: "" }];

   checkSnapShotAccessPlans( clFullName, expAccessPlans, actAccessPlans );

   println( "check result after correct analyze group success!" );

   //truncate analyze info
   var options = { Mode: 3, Collection: csName + "." + clName };
   analyze( db, options );

   //check after truncate
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

   //   var actExplains = getCommonExplain( dbclSlave, findConf);
   //   checkExplain( actExplains, expExplains );

   //query
   query( dbclPrimary, findConf, null, null, insertNums );
   //   query(dbclSlave, findConf, null, null, insertNums);

   //check out snapshot access plans
   var accessFindOption = { Collection: clFullName };
   var actAccessPlans = getCommonAccessPlans( db, accessFindOption );
   var expAccessPlans = [{ ScanType: "ixscan", IndexName: "a" }];
   checkSnapShotAccessPlans( clFullName, expAccessPlans, actAccessPlans );

   println( "check result after first truncate analyze info!" );

   //analyze with primary node
   var options = { NodeID: priNodeId };
   analyze( db, options );

   //check after correct analyze  
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

   //   var actExplains = getCommonExplain( dbclSlave, findConf);
   //   checkExplain( actExplains, expExplains );

   //query
   query( dbclPrimary, findConf, null, null, insertNums );
   //   query(dbclSlave, findConf, null, null, insertNums);

   //check out snapshot access plans
   var accessFindOption = { Collection: clFullName };
   var actAccessPlans = getCommonAccessPlans( db, accessFindOption );
   var expAccessPlans = [{ ScanType: "tbscan", IndexName: "" }];
   checkSnapShotAccessPlans( clFullName, expAccessPlans, actAccessPlans );

   println( "check result after correct analyze node success!" );

   //truncate analyze info
   var options = { Mode: 3, Collection: csName + "." + clName };
   analyze( db, options );

   //check after truncate   
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

   //   var actExplains = getCommonExplain( dbclSlave, findConf);
   //   checkExplain( actExplains, expExplains );

   //query
   query( dbclPrimary, findConf, null, null, insertNums );
   //   query(dbclSlave, findConf, null, null, insertNums);

   //check out snapshot access plans
   var accessFindOption = { Collection: clFullName };
   var actAccessPlans = getCommonAccessPlans( db, accessFindOption );
   var expAccessPlans = [{ ScanType: "ixscan", IndexName: "a" }];
   checkSnapShotAccessPlans( clFullName, expAccessPlans, actAccessPlans );

   println( "check result after second truncate analyze info!" );

   //analyze with primary host
   var options = { HostName: priHostname, svcname: priSvcname };
   analyze( db, options );

   //check after correct analyze 
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

   //   var actExplains = getCommonExplain( dbclSlave, findConf);
   //   checkExplain( actExplains, expExplains );

   //query
   query( dbclPrimary, findConf, null, null, insertNums );
   //   query(dbclSlave, findConf, null, null, insertNums);

   //check out snapshot access plans
   var accessFindOption = { Collection: clFullName };
   var actAccessPlans = getCommonAccessPlans( db, accessFindOption );
   var expAccessPlans = [{ ScanType: "tbscan", IndexName: "" }];
   checkSnapShotAccessPlans( clFullName, expAccessPlans, actAccessPlans );

   println( "check result after correct analyze host success!" );

   //check error analyze, location : slave node
   //skip one group one node
   if( primaryPos !== slavePos )
   {
      var options = [{ NodeID: slaveNodeId },
      { HostName: slaveHostname, svcname: slaveSvcname }];

      for( var i in options )
      {
         checkAnalyzeResult( options[i] );
      }

      println( "check error analyze success!" );
   }

   db1.close();
   commDropCS( db, csName, true, "drop CS in the end" );
}

function checkAnalyzeResult ( options )
{
   try
   {
      db.analyze( options );
      throw new Error( "NEED ANALYZE FAILED" );
   }
   catch( e )
   {
      if( "-264" !== e.message )
      {
         throw e;
      }
   }
}
