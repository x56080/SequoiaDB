/******************************************** 
@description : hint
@testcase    : seqDB-22179 
@author      : XiaoNi Huang 2020-02-24
*********************************************/
main();

function main ()
{
   var clName = "cl";
   var cl = db.getCollection( clName );
   cl.drop();

   // ready data
   var docs = [{ "_id": 1, "a": 1, "b": 1 },
   { "_id": 2, "a": 4, "b": 2 },
   { "_id": 3, "a": 3, "b": 3 },
   { "_id": 4, "a": 2, "b": 4 }];
   cl.insert( docs );

   // create index
   cl.createIndex( { "a": 1 }, { "name": "aIdx" } );

   // find.hint by key
   var rc = cl.find( { "b": 1 } ).hint( { "a": 1 } );
   checkResults( rc, ["[{\"_id\":1,\"a\":1,\"b\":1}]"] );
   // find.hint by index name
   var rc = cl.find( { "b": 1 } ).hint( "aIdx" );
   checkResults( rc, ["[{\"_id\":1,\"a\":1,\"b\":1}]"] );

   // count by hint, index exist
   var rc = cl.count( { "a": { "$gt": 1 } }, { "hint": "aIdx" } );
   assert.eq( rc, 3 );
   // count by hint, index not exist
   var rc = cl.count( { "a": { "$gt": 1 } }, { "hint": "notExistIndex" } );
   assert.eq( rc, 3 );


   cl.drop();
}

function checkResults ( rc, expDocs )
{
   var docs = new Array();
   while( rc.hasNext() )
   {
      var doc = rc.next();
      docs.push( doc );
   }
   assert.eq( JSON.stringify( docs ), expDocs );
}