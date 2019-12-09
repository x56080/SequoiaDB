/************************************
*@Description: ShardingKey can not be updated.
*@author:      zhaoyu
*@createdate:  2016.5.19
**************************************/
function main ()
{
   //clear environment before test;
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop CL in the beginning" );

   //check test environment before split
   try
   {
      //standalone can not split
      if( true == commIsStandalone( db ) )
      {
         println( "run mode is standalone" );
         return;
      }
      //less two groups,can not split
      var allGroupName = getGroupName( db );
      if( 1 === allGroupName.length )
      {
         println( "--least two groups" );
         return;
      }
   }
   catch( e )
   {
      throw e;
   }

   //create cl, ShardingKey is age
   var ClOption = { ShardingKey: { "age": 1 }, ShardingType: "range", ReplSize: 0 };
   var dbcl = commCreateCLByOption( db, COMMCSNAME, COMMCLNAME, ClOption, true, true );

   //insert data
   var doc1 = [{ age: 1 }, { age: 2 }];
   insertData( dbcl, doc1 );

   //update common data
   var updateCondition1 = { $unset: { age: "" } };
   updateData( dbcl, updateCondition1 );

   //check result
   var expRecs1 = [{ age: 1 }, { age: 2 }];
   checkResult( dbcl, null, null, expRecs1, { _id: 1 } );

   //insert data
   var doc2 = [{ age: [1, 2, 3] }];
   insertData( dbcl, doc2 );

   //update common data
   var updateCondition2 = { $addtoset: { age: [3, 4, 5, 6] } };
   updateData( dbcl, updateCondition2 );

   //check result
   var expRecs1 = [{ age: 1 }, { age: 2 }, { age: [1, 2, 3] }];
   checkResult( dbcl, null, null, expRecs1, { _id: 1 } );
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
;