/************************************
*@Description: access plan should be cleared when the old mainCL name be rename
*@author:      luweikang
*@createDate:  2019.04.09
*@testlinkCase:seqDB-17010
**************************************/

main();

function main ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   println( "---begin rename cl test---" );
   var mainCSName = "maincs17010";
   var subCSName = "subcs17010";
   var mainCLName = "maincl17010";
   var newMainCLName = "newmaincl17010";
   var subCLName = "subcl17010";

   //create mainCSCL and subCSCL, query the main cl to generate an access plan
   println( "---create mainCSCL and subCSCL---" );
   var mainCL = db.createCS( mainCSName ).createCL( mainCLName, { "IsMainCL": true, "ReplSize": 0, "ShardingKey": { "a": 1 } } );
   var subCL = db.createCS( subCSName ).createCL( subCLName, { "ShardingKey": { b: 1 } } );

   mainCL.attachCL( subCSName + "." + subCLName, { "LowBound": { "a": 0 }, "UpBound": { "a": 10 } } );
   mainCL.insert( { "_id": 1, "a": 1, "b": 1 } );
   var cur = mainCL.find();
   while( cur.next() )
   {
      var record = cur.current();
   }
   cur.close();

   //check the old mainCS access plan is exist
   println( "---check mainCL accessplans---" );
   var matcher = { "Collection": mainCSName + "." + mainCLName };
   var selector = { "CollectionSpace": 1 };
   var snapshotCur = db.snapshot( 11, matcher, selector );
   if( snapshotCur.next() )
   {
      var csName = snapshotCur.current().toObj().CollectionSpace;
      if( csName != mainCSName )
      {
         throw buildException( "checkAccessplans", "", "check old cl accessplans exist", mainCSName, csName )
      }
      snapshotCur.close();
   }
   else
   {
      throw buildException( "checkAccessplans", "", "check old cl accessplans exist", "exist", "notExist" );
   }

   //rename the mainCL
   println( "---rename mainCL name---" );
   db.getCS( mainCSName ).renameCL( mainCLName, newMainCLName );
   var newMainCL = db.getCS( mainCSName ).getCL( newMainCLName );
   cur = newMainCL.find();
   while( cur.next() )
   {
      var record = cur.current();
   }
   cur.close();

   //check the old mainCL access plan shuold be not exist
   println( "---check old mainCL accessplans---" );
   snapshotCur = db.snapshot( 11, matcher, selector );
   if( snapshotCur.next() )
   {
      throw buildException( "checkAccessplans", "", "check old cl accessplans exist", "notExist", snapshotCur.current() );
   }
   snapshotCur.close();

   //check the new mainCL access plan shuold be not exist
   println( "---check new mainCL accessplans---" );
   var newMatcher = { "Collection": mainCSName + "." + newMainCLName };
   snapshotCur = db.snapshot( 11, newMatcher, selector );
   if( snapshotCur.next() )
   {
      var csName = snapshotCur.current().toObj().CollectionSpace;
      if( csName != mainCSName )
      {
         throw buildException( "checkAccessplans", "", "check old cl accessplans exist", mainCSName, csName )
      }
      snapshotCur.close();
   }
   else
   {
      throw buildException( "checkAccessplans", "", "check new cl accessplans exist", "exist", "notExist" );
   }

   println( "---cleanUp in the end---" );
   db.dropCS( mainCSName );
   db.dropCS( subCSName );
}
