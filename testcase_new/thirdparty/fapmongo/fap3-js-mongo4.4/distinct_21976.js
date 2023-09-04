/******************************************** 
@description : distinct
@testcase    : seqDB-21976
@author      : XiaoNi Huang 2020-02-24
*********************************************/
main();

function main ()
{
   var clName = "cl21967";
   var cl = db.getCollection( clName );
   cl.drop();

   // ready data
   var docs = [{ "_id": 1, "a": 1, "b": 1 },
   { "_id": 4, "a": 4, "b": 1 },
   { "_id": 3, "a": 3, "b": 2 },
   { "_id": 2, "a": 2, "b": 2 },
   { "_id": 5, "a": 5, "b": 3 }];
   cl.insert( docs );

   // distinct
   var rc = cl.distinct( "b" );
   assert.eq( rc, [1, 2, 3] );
   // distinct by match, exist repeat value
   var rc = cl.distinct( "b", { "b": { "$lt": 3 } } );
   assert.eq( rc, [1, 2] );
   // distinct by match, not exist repeat value
   var rc = cl.distinct( "b", { "a": { "$gt": 3 } } );
   assert.eq( rc, [1, 3] );
   // distinct, not match docs
   var rc = cl.distinct( "b", { "b": { "$gt": 1000 } } );
   assert.eq( rc, [] );
   // distinct, not exist field
   var rc = cl.distinct( "notExistField" );
   assert.eq( rc, [] );
   cl.remove( {} );

   // cl empty, distinct
   cl.remove( {} );
   var rc = cl.distinct( "a" );
   assert.eq( rc, [] );

   // field is object
   cl.insert( [{ "_id": 6, "a": 6, "b": { "b1": 1, "b2": 1 } },
   { "_id": 7, "a": 7, "b": { "b1": 2, "b2": 2 } },
   { "_id": 8, "a": 8, "b": { "b1": 1, "b2": 3 } }] );
   var rc = cl.distinct( "b.b1" );
   assert.eq( rc, [1, 2] );
   cl.remove( {} );

   cl.drop();
}