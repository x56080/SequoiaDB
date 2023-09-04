/******************************************** 
@description : insertMany/updateMany/find/delete
@testcase    : seqDB-21964
@author      : XiaoNi Huang 2020-02-24
*********************************************/
main();

function main ()
{
   var clName = "cl21964";
   var cl = db.getCollection( clName );
   cl.drop();

   // ready data
   var docs = [{ "_id": 1, "a": 1, "b": "insertMany" },
   { "_id": 2, "a": 2, "b": "insertMany" },
   { "_id": 3, "a": 3, "b": "insertMany" },
   { "_id": 4, "a": 4, "b": "insertMany" }];

   // cl.insertMany([documents],{writeConcern:<document>,ordered:<boolean>})
   // insert [documents]
   var rc = cl.insertMany( [docs[0], docs[1]] );
   assert.eq( rc, { "acknowledged": true, "insertedIds": [1, 2] } );

   // insert [documents], point writeConcern and ordered, ordered:false
   var rc = cl.insertMany( [docs[3], docs[2]], { "writeConcern": { "w": 1 }, "ordered": false } );
   assert.eq( rc, { "acknowledged": true, "insertedIds": [4, 3] } );

   /* ordered:true, sdb not support
   // insert [documents], point writeConcern and ordered, ordered:true
   var rc = cl.insertMany( [{ "_id": 6, "a": 6, "b": "insertMany" }, { "_id": 5, "a": 5, "b": "insertMany" }], { "writeConcern": { "w": 1 }, "ordered": true } );
   assert.eq( rc, { "acknowledged": true, "insertedIds":[ 5, 6 ] } );
   */

   // insert []
   var rc = cl.insertMany( [] );
   assert.eq( rc, { "acknowledged": true, "insertedIds": [] } );

   // check results
   var rc = cl.find().sort( { "a": 1 } );
   var expDocs = "[{\"_id\":1,\"a\":1,\"b\":\"insertMany\"},{\"_id\":2,\"a\":2,\"b\":\"insertMany\"},{\"_id\":3,\"a\":3,\"b\":\"insertMany\"},{\"_id\":4,\"a\":4,\"b\":\"insertMany\"}]";
   checkResults( rc, expDocs );


   // cl.updateMany(<filter>,<update>,{upsert:<boolean>,writeConcern:<document>,collation:<document>})
   var rc = cl.updateMany( { "a": { "$lt": 4 } }, { "$set": { "b": "test" } } );
   assert.eq( rc, { "acknowledged": true, "matchedCount": 3, "modifiedCount": 3 } );

   // cl.updateMany(<filter>,<update>, {multi:true})
   var rc = cl.updateMany( { "a": { "$exists": 1 } }, { "$set": { "u2": 1 } }, { "multi": true } );
   assert.eq( rc, { "acknowledged": true, "matchedCount": 4, "modifiedCount": 4 } );

   // cl.updateMany(<filter>,<update>, {writeConcern:<document>,collation:<document>})
   var rc = cl.updateMany( { "_id": 2 }, { "$set": { "u3": 1 } }, { "writeConcern": { "j": true }, "collation": { "locale": "fr" } } );
   assert.eq( rc, { "acknowledged": true, "matchedCount": 1, "modifiedCount": 1 } );

   // cl.update(<filter>,<update>})
   var rc = cl.update( {}, { "$set": { "u4": 1 } }, { "multi": true } );
   assert.eq( rc, { "nMatched": 4, "nUpserted": 0, "nModified": 4 } );

   // check results
   var rc = cl.find().sort( { "a": 1 } );
   var expDocs = "[{\"_id\":1,\"a\":1,\"b\":\"test\",\"u2\":1,\"u4\":1},{\"_id\":2,\"a\":2,\"b\":\"test\",\"u2\":1,\"u3\":1,\"u4\":1},{\"_id\":3,\"a\":3,\"b\":\"test\",\"u2\":1,\"u4\":1},{\"_id\":4,\"a\":4,\"b\":\"insertMany\",\"u2\":1,\"u4\":1}]";
   checkResults( rc, expDocs );


   // cl.deleteMany(<filter>,{writeConcern:<document>,collation:<document>})
   // param: filter
   var rc = cl.deleteMany( { "a": { "$lt": 3 } } );
   assert.eq( rc, { "acknowledged": true, "deletedCount": 2 } );
   checkResults( cl.find().sort( { "a": 1 } ), "[{\"_id\":3,\"a\":3,\"b\":\"test\",\"u2\":1,\"u4\":1},{\"_id\":4,\"a\":4,\"b\":\"insertMany\",\"u2\":1,\"u4\":1}]" );

   // all param
   var rc = cl.deleteMany( { "a": { "$lte": 3 } }, { "writeConcern": { "j": true }, "collation": { "locale": "fr" } } );
   assert.eq( rc, { "acknowledged": true, "deletedCount": 1 } );
   checkResults( cl.find(), "[{\"_id\":4,\"a\":4,\"b\":\"insertMany\",\"u2\":1,\"u4\":1}]" );

   // deleteMany({})
   cl.insert( { "a": 10 } );
   var cnt = NumberInt( cl.count() );
   var rc = cl.deleteMany( {} );
   assert.eq( rc, { "acknowledged": true, "deletedCount": cnt } );
   var rc = cl.find();
   checkResults( rc, "[]" );


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