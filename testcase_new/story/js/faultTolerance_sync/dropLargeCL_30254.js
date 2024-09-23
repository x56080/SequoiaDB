/******************************************************************************
*@Description : seqDB-30254:删除大数据集合后检测节点状态
*@author:      linsuqiang
*@createdate:  2024.09.05
******************************************************************************/
testConf.skipStandAlone=true;
main( test );

function test ()
{
   var csName = "cs_30254";
   var clName = "cl_30254";

   commDropCS( db, csName );

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
   commCreateCL( db, csName, clName, { Group: groupName, ReplSize: 1 } );
   // prevent fast dropCL by dropCS
   commCreateCL( db, csName, clName + "_reserved", { Group: groupName } );
   var cl = db.getCS( csName ).getCL( clName );
   var recArray = [];
   for (var i = 0; i < 1000; i++) {
      recArray.push( { a: i, b: i, c: i } );
   }
   cl.insert( recArray );
   db.updateConf( { ftconfirmperiod: 6 }, { GroupName: groupName } );

   var master = null;
   var slave = null;
   try
   {
      master = db.getRG( groupName ).getMaster().connect();
      slave = db.getRG( groupName ).getSlave().connect();
      var option = new SdbTraceOption().breakPoints("_dmsStorageDataCommon::dropCollection");
      slave.traceOn( 1000, option );

      cs.dropCL( clName );

      for (var i = 0; i < 12; i++)
      {
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
      db.deleteConf( { ftconfirmperiod: '' }, { GroupName: groupName } );
   }
}

