/******************************************************************************
*@Description : seqDB-34200:删除大数据集合，并发写入，检测节点状态
*@author:      linsuqiang
*@createdate:  2024.09.05
******************************************************************************/
testConf.skipStandAlone=true;
main( test );

function test ()
{
   var csName = "cs_34200";
   var clName = "cl_34200";
   var csName2 = csName + "_2";
   var clName2 = clName + "_2";

   commDropCS( db, csName );
   commDropCS( db, csName2 );

   var groupsArray = commGetGroups( db, false, "", true, true, true );
   var groupName = null;
   for (var i = 0; i < groupsArray.length; i++)
   {
      if (groupsArray[i].length >= 2)
      {
         groupName = groupsArray[i][0].GroupName;
         break;
      }
   }
   if (!groupName)
   {
      println("skip for no slave nodes");
      return;
   }

   var cs = commCreateCS( db, csName );
   var cl = commCreateCL( db, csName, clName, { Group: groupName, ReplSize: 1 } );
   // prevent fast dropCL by dropCS
   var cl = commCreateCL( db, csName, clName + "_reserved", { Group: groupName } );
   var recArray = [];
   for (var i = 0; i < 1000; i++) {
      recArray.push( { a: i, b: i, c: i } );
   }
   cl.insert( recArray );

   commCreateCS( db, csName2 );
   var cl2 = commCreateCL( db, csName2, clName2, { Group: groupName, ReplSize: 1 } );

   db.updateConf(
      {
         ftconfirmperiod: 6,
         ftmask: "ALL",
         ftslownodethreshold: 1,
         ftslownodeincrement: 0 
      },
      { GroupName: groupName }
   );
   commCheckLSN( db, groupName );

   var master = null;
   var slave = null;
   try
   {
      master = db.getRG( groupName ).getMaster().connect();
      slave = db.getRG( groupName ).getSlave().connect();
      var option = new SdbTraceOption()
            .breakPoints("_dmsStorageDataCommon::dropCollection");
      var longString = getRandomString( 1024 * 1024 );
      slave.traceOn( 1000, option );

      cs.dropCL( clName );

      for (var i = 0; i < 12; i++)
      {
         cl2.insert( { a: longString } );
         var selector = { NodeName: '', FTStatus: '', CurrentLSN: '', CompleteLSN: '' };
         var snap = master.snapshot( SDB_SNAP_DATABASE, {}, selector );
         println( "master:" );
         var res = snap.next().toObj();
         println( JSON.stringify( res ) );

         snap = slave.snapshot( SDB_SNAP_DATABASE, {}, selector );
         println( "slave:" );
         res = snap.next().toObj();
         println( JSON.stringify( res ) );
         println();

         assert.equal( res.FTStatus, "" );
         sleep(1000);
      }

      commDropCS( db, csName );
      commDropCS( db, csName2 );
   }
   finally
   {
      if (slave)
      {
         slave.traceOff("");
         slave.close();
      }
      if (master)
      {
         master.close();
      }
      db.deleteConf(
         {
            ftconfirmperiod: '',
            ftmask: '',
            ftslownodethreshold: '',
            ftslownodeincrement: ''
         },
         { GroupName: groupName }
      );
   }
}

