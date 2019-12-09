/************************************
*@Description: access plan should be cleared when the old mainCS name be rename
*@author:      luweikang
*@createDate:  2019.04.09
*@testlinkCase:seqDB-16864
**************************************/

main();

function main ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   println( "---begin rename cs test---" );
   var mainCSName = "maincs16864";
   var newMainCSName = "newmaincs16864";
   var subCSName = "subcs16864";
   var mainCLName = "maincl16864";
   var subCLName = "subcl16864";

   //create mainCSCL and subCSCL, query the main cs to generate an access plan
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
   println( "---check mainCS accessplans---" );
   var matcher = { "CollectionSpace": mainCSName };
   var selector = { "Collection": 1 };
   var snapshotCur = db.snapshot( 11, matcher, selector );
   if( snapshotCur.next() )
   {
      var clName = snapshotCur.current().toObj().Collection;
      if( clName != mainCSName + "." + mainCLName )
      {
         throw buildException( "checkAccessplans", "", "check old cs accessplans exist", mainCSName + "." + mainCLName, clName )
      }
      snapshotCur.close();
   }
   else
   {
      throw buildException( "checkAccessplans", "", "check old cs accessplans exist", "exist", "notExist" );
   }

   //rename the mainCS
   println( "---rename mainCS name---" );
   db.renameCS( mainCSName, newMainCSName );
   var newMainCL = db.getCS( newMainCSName ).getCL( mainCLName );
   cur = newMainCL.find();
   while( cur.next() )
   {
      var record = cur.current();
   }
   cur.close();

   //check the old mainCS access plan shuold be not exist
   println( "---check old mainCS accessplans---" );
   snapshotCur = db.snapshot( 11, matcher, selector );
   if( snapshotCur.next() )
   {
      throw buildException( "checkAccessplans", "", "check old cs accessplans exist", "notExist", snapshotCur.current() );
   }
   snapshotCur.close();

   //check the new mainCS access plan shuold be not exist
   println( "---check new mainCS accessplans---" );
   var newMatcher = { "CollectionSpace": newMainCSName };
   snapshotCur = db.snapshot( 11, newMatcher, selector );
   if( snapshotCur.next() )
   {
      var clName = snapshotCur.current().toObj().Collection;
      if( clName != newMainCSName + "." + mainCLName )
      {
         throw buildException( "checkAccessplans", "", "check new cs accessplans exist", newMainCSName + "." + mainCLName, clName )
      }
      snapshotCur.close();
   }
   else
   {
      throw buildException( "checkAccessplans", "", "check new cs accessplans exist", "exist", "notExist" );
   }

   println( "---cleanUp in the end---" );
   db.dropCS( newMainCSName );
   db.dropCS( subCSName );
}
