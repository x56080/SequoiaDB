/******************************************************************************
*@Description : seqDB-34199:drop 大数据 CS 后检测节点状态
*@author:      linsuqiang
*@createdate:  2024.09.05
******************************************************************************/
testConf.skipStandAlone=true;
main( test );

function test ()
{
   var csName = "cs_34199";
   var clName = "cl_34199";

   commDropCS( db, csName );

   var groupsArray = commGetGroups( db, false, "", true, true, true );
   var groupName = null;
   for (var i = 0; i < groupsArray.length; i++)
   {
      if (groupsArray[i][0].Length >= 2)
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
   var cl = db.getCS( csName ).getCL( clName );
   var recArray = [];
   for (var i = 0; i < 1000; i++) {
      recArray.push( { a: i, b: i, c: i } );
   }
   cl.insert( recArray );
   commCheckLSN( db, groupName );
   db.updateConf( { ftconfirmperiod: 6 }, { GroupName: groupName } );

   var master = null;
   var slave = null;
   try
   {
      master = db.getRG( groupName ).getMaster().connect();
      slave = db.getRG( groupName ).getSlave().connect();
      var option = new SdbTraceOption().breakPoints("ossDelete");
      slave.traceOn( 1000, option );

      commDropCS( db, csName );

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

