/************************************
*@Description: upsert object(exist or not exist) use operator push
*@author:      zhaoyu
*@createdate:  2016.5.19
**************************************/
function main ()
{
   //clean environment before test
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop CL in the beginning" );

   //create cl
   var dbcl = commCreateCL( db, COMMCSNAME, COMMCLNAME );

   //insert object
   var Doc = { a: 1 };
   insertData( dbcl, Doc );

   //upsert any object when match nothing,use matches and
   var upsertCondition1 = {
      $push: {
         object1: 40, object2: 10,
         object3: 2,
         "object4.1": 30, "object5.1": 100,
         object14: 11
      }
   };
   var findCondition1 = {
      $and: [{ object1: [50, [30, 50, [90, 40], 80], 25, 40, 15] },
      { object2: [5, 7, 9, 2, -8, 4] },
      { object3: { $et: { $decimal: "2" } } },
      { object4: { $all: [50, [30, 50, [90, 40], 80], 25, 40, 15] } },
      { object5: { $all: [50, [30, 50, [90, 40], 80], 25, 40, 15] } },
      { object6: [5, [9, 8, 4, 3], 10] },
      { c: { $gt: 100 } }]
   };
   upsertData( dbcl, upsertCondition1, findCondition1 );

   //check result
   var expRecs1 = [{
      object1: [50, [30, 50, [90, 40], 80], 25, 40, 15, 40],
      object2: [5, 7, 9, 2, -8, 4, 10],
      object3: { $decimal: "2" },
      object4: [50, [30, 50, [90, 40], 80, 30], 25, 40, 15],
      object5: [50, [30, 50, [90, 40], 80, 100], 25, 40, 15],
      object6: [5, [9, 8, 4, 3], 10],
      object14: [11]
   },
   { a: 1 }];
   checkResult( dbcl, null, null, expRecs1, { a: 1 } );

   //upsert any object when match nothing,use matches or
   var upsertCondition2 = {
      $push: {
         object1: 40, object2: 10,
         object3: 2,
         "object4.1": 30, "object5.1": 100,
         object14: 11
      }
   };
   var findCondition2 = {
      $or: [{ object2: [10, 13, 15] },
      { object3: { $et: { $decimal: "22" } } },
      { object4: { $all: [50, 55, 45] } },
      { c: { $gt: 100 } }]
   };
   upsertData( dbcl, upsertCondition2, findCondition2 );

   //check result
   var expRecs2 = [{
      object1: [50, [30, 50, [90, 40], 80], 25, 40, 15, 40],
      object2: [5, 7, 9, 2, -8, 4, 10],
      object3: { $decimal: "2" },
      object4: [50, [30, 50, [90, 40], 80, 30], 25, 40, 15],
      object5: [50, [30, 50, [90, 40], 80, 100], 25, 40, 15],
      object6: [5, [9, 8, 4, 3], 10],
      object14: [11]
   },
   {
      object1: [40],
      object2: [10],
      object3: [2],
      object4: { 1: [30] },
      object5: { 1: [100] },
      object14: [11]
   },
   { a: 1 }];
   checkResult( dbcl, null, null, expRecs2, { a: 1 } );

   //delete all data
   deleteData( dbcl, null );

   //upsert any object when match nothing,use matches not
   var upsertCondition3 = {
      $push: {
         object1: 40, object2: 10,
         object3: 2,
         "object4.1": 30, "object5.1": 100,
         object14: 11
      }
   };
   var findCondition3 = {
      $not: [{ object2: [10, 13, 15] },
      { object3: { $et: { $decimal: "22" } } },
      { object4: { $all: [50, 55, 45] } },
      { c: { $gt: 100 } }]
   };
   upsertData( dbcl, upsertCondition3, findCondition3 );

   //check result
   var expRecs3 = [{
      object1: [40],
      object2: [10],
      object3: [2],
      object4: { 1: [30] },
      object5: { 1: [100] },
      object14: [11]
   }];
   checkResult( dbcl, null, null, expRecs3, { a: 1 } );
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