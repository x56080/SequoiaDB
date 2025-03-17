/******************************************************************************
*@Description : seqDB-34287:长事务删除与后台回收
*@Description : seqDB-34288:后台回收与 DDL 并发
*@author:      linsuqiang
*@createdate:  2025.03.05
******************************************************************************/
testConf.skipStandAlone=true;
main( test );

function test ()
{
   var groupsArray = commGetGroups( db, false, "", true, true, true );
   var groupName = groupName = groupsArray[0][0].GroupName;
   db.updateConf( { recordrecycledelay: 0, syncinterval: 1 }, { GroupName: groupName } );

   try
   {
      var db1 = new Sdb( COORDHOSTNAME, COORDSVCNAME );
      const csName = "cs_34287";
      const clName = "cl_34287";
      const totalRecords = 1000;

      commDropCS( db1, csName );

      var cs = commCreateCS( db1, csName );
      commCreateCL( db1, csName, clName, { Group: groupName } );
      var cl = db1.getCS( csName ).getCL( clName );
      var recArray = [];
      for (var i = 0; i < totalRecords; i++) {
         recArray.push( { a: i, b: i, c: i } );
      }
      cl.insert( recArray );

      var checker = new RecycleChecker( db1, csName, clName, groupName );

      db1.transBegin();
      cl.remove();
      checker.checkTotalDeletingRecords( totalRecords, 15 );
      // commit at the last



      const csName2 = "cs_34288_1";
      const clName2 = "cl_34288_1";
      const totalRecords2 = 1000;

      commDropCS( db, csName2 );

      var cs2 = commCreateCS( db, csName2 );
      commCreateCL( db, csName2, clName2, { Group: groupName } );
      var cl2 = db.getCS( csName2 ).getCL( clName2 );
      var recArray2 = [];
      for (var i = 0; i < totalRecords2; i++) {
         recArray2.push( { a: i, b: i, c: i } );
      }
      cl2.insert( recArray2 );

      db.transBegin();
      cl2.remove();
      db.transCommit();



      const csName3 = "cs_34288_2";
      const clName3 = "cl_34288_2";
      const totalRecords3 = 1000;

      commDropCS( db, csName3 );

      var cs3 = commCreateCS( db, csName3 );
      commCreateCL( db, csName3, clName3, { Group: groupName } );
      var cl3 = db.getCS( csName3 ).getCL( clName3 );
      var recArray3 = [];
      for (var i = 0; i < totalRecords3; i++) {
         recArray3.push( { a: i, b: i, c: i } );
      }
      cl3.insert( recArray3 );

      db.transBegin();
      cl3.remove();
      db.transCommit();



      const csName4 = "cs_34288_3";
      const clName4 = "cl_34288_3";
      const totalRecords4 = 1000;

      commDropCS( db, csName4 );

      var cs4 = commCreateCS( db, csName4 );
      commCreateCL( db, csName4, clName4, { Group: groupName } );
      var cl4 = db.getCS( csName4 ).getCL( clName4 );
      var recArray4 = [];
      for (var i = 0; i < totalRecords4; i++) {
         recArray4.push( { a: i, b: i, c: i } );
      }
      cl4.insert( recArray4 );

      db.transBegin();
      cl4.remove();
      db.transCommit();



      dataNode = db.getRG( groupName ).getMaster().connect();
      var option = new SdbTraceOption().breakPoints( "_clsRecycleRecordJob::_doRecycleRecordJobs" );
      dataNode.traceOn( 1024, option );

      db.updateConf( { recordrecycledelay: 1, syncinterval: 1 }, { GroupName: groupName } );

      sleep( 60 * 1000 ); // wait to ensure break points hit

      db.dropCS( csName2 );
      cs3.dropCL( clName3 );
      cl4.truncate();

      dataNode.traceOff("");


      checker.checkTotalDeletingRecords( totalRecords, 90 );
      sleep( 20 * 1000 ); // sleep to cover retry locking
      db1.transCommit();
      checker.checkTotalDeletingRecords( 0, 90 );

      checker.cleanUp();
      commDropCS( db, csName );

      commDropCS( db, csName3 );
      commDropCS( db, csName4 );
   }
   finally
   {
      db.deleteConf( { recordrecycledelay: '', syncinterval: '' }, { GroupName: groupName } );
      if (dataNode)
      {
         dataNode.traceOff("");
      }
   }
}
