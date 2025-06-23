/******************************************************************************
*@Description : seqDB-34302:旧版本的 overflow 升级后回收
*@author:      linsuqiang
*@createdate:  2025.06.17
******************************************************************************/
testConf.skipStandAlone=true;
main( test );

function test ()
{
   var groupsArray = commGetGroups( db, false, "", true, true, true );
   var groupName = groupName = groupsArray[0][0].GroupName;
   // update configure option to walk the old version way
   db.updateConf( { recordrecycledelay: -1 }, { GroupName: groupName } );

   try
   {
      const csName = "cs_34302";
      const clName = "cl_34302";
      const totalRecords = 12000;
      db.setSessionAttr({ TransMaxLockNum: -1 });

      commDropCS( db, csName );

      var cs = commCreateCS( db, csName );
      commCreateCL( db, csName, clName, { Group: groupName, Compressed: false } );
      var cl = db.getCS( csName ).getCL( clName );
      var recArray = [];
      for (var i = 0; i < totalRecords; i++) {
         recArray.push( { a: i, b: i, c: i } );
      }
      cl.insert( recArray );
      var checker = new RecycleChecker( db, csName, clName, groupName );
      cl.update( { $set: { b: "fsldjfeiflsdjflsdjflsdjflsdfjlsdfjsl" } },
                 { a: { $gte: 9000 } } );
      checker.checkTotalOverflowRecords( 3000 );
      db.transBegin();
      cl.remove( { a: { $gte: 4200, $lte: 10800 } } );
      db.transCommit();

      checker.checkTotalDeletingRecords( 0, 15 );

      // update configure option to walk the upgraded way
      db.updateConf( { recordrecycledelay: 1 } );
      cl.update( { $null: { a: 1 } } );
      checker.checkTotalDeletingRecords( 0, 120 );

      var rec = [];
      for ( var i = 12000 ; i < 18000 ; ++i )
      {
         rec.push( {a:i} );
      }
      cl.insert( rec );

      cl.update( { $set: { b: "fsldjfeiflsdjflsdjflsdjflsdfjlsdfjsl" } },
                 { a: { $gte: 15000 } } );
      checker.checkTotalOverflowRecords( 4199 );

      cl.update( { $null: { a: 1 } } );

      commDropCS( db, csName );
   }
   finally
   {
      db.setSessionAttr({ TransMaxLockNum: 10000 });
      db.deleteConf( { recordrecycledelay: '' }, { GroupName: groupName } );
   }
}
