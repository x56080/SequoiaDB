/******************************************************************************
*@Description : seqDB-34286:事务回滚提交与后台回收
*@author:      linsuqiang
*@createdate:  2025.03.05
******************************************************************************/
testConf.skipStandAlone=true;
main( test );

function test ()
{
   var groupsArray = commGetGroups( db, false, "", true, true, true );
   var groupName = groupName = groupsArray[0][0].GroupName;
   db.updateConf( { recordrecycledelay: 1, syncinterval: 1 }, { GroupName: groupName } );

   try
   {
      const csName = "cs_34286";
      const clName = "cl_34286";
      const totalRecords = 1000;

      commDropCS( db, csName );

      var cs = commCreateCS( db, csName );
      commCreateCL( db, csName, clName, { Group: groupName } );
      var cl = db.getCS( csName ).getCL( clName );
      var recArray = [];
      for (var i = 0; i < totalRecords; i++) {
         recArray.push( { a: i, b: i, c: i } );
      }
      cl.insert( recArray );

      var checker = new RecycleChecker( db, csName, clName, groupName );

      cl.insert({ a: 10086 });
      cl.remove({ a: 10086 }); // not in transaction
      checker.checkTotalDeletingRecords( 0, 15 );

      db.transBegin();
      cl.remove();
      checker.checkTotalDeletingRecords( totalRecords, 15 );
      db.transRollback();
      checker.checkTotalDeletingRecords( 0, 15 );

      db.transBegin();
      cl.remove();
      checker.checkTotalDeletingRecords( totalRecords, 15 );
      db.transCommit();
      // check recycle at last



      const csName2 = "cs_34286_2";
      const clName2 = "cl_34286_2";
      const totalRecords2 = 5000;

      commDropCS( db, csName2 );

      // var groupsArray = commGetGroups( db, false, "", true, true, true );
      // var groupName = groupName = groupsArray[0][0].GroupName;

      var cs2 = commCreateCS( db, csName2 );
      commCreateCL( db, csName2, clName2, { Group: groupName } );
      var cl2 = db.getCS( csName2 ).getCL( clName2 );
      var recArray2 = [];
      for (var i = 0; i < totalRecords2; i++) {
         recArray2.push( { a: i, b: i, c: i } );
      }
      cl2.insert( recArray2 );

      const maxLockNum = 2000;
      db.setSessionAttr({ TransMaxLockNum: maxLockNum });

      var checker2 = new RecycleChecker( db, csName2, clName2, groupName );

      db.transBegin();
      cl2.remove();
      checker2.checkTotalDeletingRecords( maxLockNum + 1, 15 );
      db.transRollback();
      checker2.checkTotalDeletingRecords( 0, 15 );

      db.transBegin();
      cl2.remove();
      checker2.checkTotalDeletingRecords( maxLockNum + 1, 15 );
      db.transCommit();
      // check recycle at last

      db.setSessionAttr({ TransMaxLockNum: 10000 });



      const csName3 = "cs_34286_3";
      const clName3 = "cl_34286_3";
      const totalRecords3 = 1000;

      commDropCS( db, csName3 );

      // var groupsArray = commGetGroups( db, false, "", true, true, true );
      // var groupName = groupName = groupsArray[0][0].GroupName;

      var cs3 = commCreateCS( db, csName3 );
      commCreateCL( db, csName3, clName3, { Group: groupName } );
      var cl3 = db.getCS( csName3 ).getCL( clName3 );
      var recArray3 = [];
      for (var i = 0; i < totalRecords3; i++) {
         recArray3.push( { a: i, b: i, c: i } );
      }
      cl3.insert( recArray3 );
      cl3.update({ $set: { a: "this_is_a_long_long_string_to_construct_overflow" } });

      var checker3 = new RecycleChecker( db, csName3, clName3, groupName );

      db.transBegin();
      cl3.remove();
      checker3.checkTotalDeletingRecords( totalRecords3, 15 );
      db.transRollback();
      checker3.checkTotalDeletingRecords( 0, 15 );

      db.transBegin();
      var cursor = cl3.find().remove();
      while (cursor.next() != null) {}
      cursor.close();
      checker3.checkTotalDeletingRecords( totalRecords3, 15 );
      db.transCommit();
      // check recycle at last



      checker.checkTotalDeletingRecords( 0, 90 );
      checker.cleanUp();
      commDropCS( db, csName );

      checker2.checkTotalDeletingRecords( 0, 90 );
      checker2.cleanUp();
      commDropCS( db, csName2 );

      checker3.checkTotalDeletingRecords( 0, 90 );
      checker3.cleanUp();
      commDropCS( db, csName3 );
   }
   finally
   {
      db.deleteConf( { recordrecycledelay: '', syncinterval: '' }, { GroupName: groupName } );
   }
}
