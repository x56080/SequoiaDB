/******************************************** 
@description : insertOne/updateOne/findOne/deleteOne
@testcase    : seqDB-21965
@author      : XiaoNi Huang 2020-02-24
*********************************************/
main();

function main ()
{
   var clName = "cl21965";
   var cl = db.getCollection( clName );
   cl.drop();

   //cl.insertOne(<document>,{writeConcern:<document>})
   // insert document
   var rc = cl.insertOne( { "_id": 1, "a": 1, "b": "insertOne" } );
   assert.eq( rc, { "acknowledged": true, "insertedId": 1 } );

   // insert document, point writeConcern
   var rc = cl.insertOne( { "_id": 2, "a": 2, "b": "insertOne" }, { "writeConcern": { "w": 1 } } );
   assert.eq( rc, { "acknowledged": true, "insertedId": 2 } );

   // insert {}, random oid, delete after insert for check results
   var rc = cl.insertOne( {} );
   assert.eq( JSON.stringify( rc ).length, 70 );
   var oid = rc.insertedId;
   var rc = cl.deleteOne( { "_id": oid } );
   assert.eq( rc, { "acknowledged": true, "deletedCount": 1 } );

   // check results
   var rc = cl.find().sort( { "a": 1 } );
   checkResults( rc, "[{\"_id\":1,\"a\":1,\"b\":\"insertOne\"},{\"_id\":2,\"a\":2,\"b\":\"insertOne\"}]" );


   // cl.updateOne(<filter>,<update>,{upsert:<boolean>,writeConcern:<document>,collation:<document>})
   // updateOne, match multi docs
   var rc = cl.updateOne( {}, { "$set": { "b": "updateOne" } } );
   assert.eq( rc, { "acknowledged": true, "matchedCount": 1, "modifiedCount": 1 } );

   // updateOne, not match docs
   var rc = cl.updateOne( { "a": "notExist" }, { "$set": { "b": "updateOne" } } );
   assert.eq( rc, { "acknowledged": true, "matchedCount": 0, "modifiedCount": 0 } );

   // updateOne, not match docs, upsert:true
   var rc = cl.updateOne( { "a": "notExist" }, { "$set": { "_id": 3, "a": 3, "b": "updateOne" } }, { "upsert": true }, { "writeConcern": { "j": true }, "collation": { "locale": "fr" } } );
   assert.eq( rc, { "acknowledged": true, "matchedCount": 0, "modifiedCount": 0, "upsertedId": 3 } );

   // check results
   var rc = cl.find().sort( { "a": 1 } );
   checkResults( rc, "[{\"_id\":1,\"a\":1,\"b\":\"updateOne\"},{\"_id\":2,\"a\":2,\"b\":\"insertOne\"},{\"_id\":3,\"a\":3,\"b\":\"updateOne\"}]" );


   // cl.findOne(<query>,<projection>)
   // not param
   var rc = cl.findOne();
   assert.eq( rc, { "_id": 1, "a": 1, "b": "updateOne" } );

   // param: query
   var rc = cl.findOne( { "a": { "$gte": 2 } } );
   assert.eq( rc, { "_id": 2, "a": 2, "b": "insertOne" } );

   // param: query / projection
   var rc = cl.findOne( { "a": 1 }, { "a": "" } );
   assert.eq( rc, { "_id": 1, "a": 1 } );

   // not match docs
   var rc = cl.findOne( { "a": "notExist" } );
   assert.eq( rc, null );


   // cl.deleteOne(<filter>,{writeConcern:<document>,collation:<document>})
   // match multi docs
   var rc = cl.deleteOne( {} );
   assert.eq( rc, { "acknowledged": true, "deletedCount": 1 } );
   checkResults( cl.find(), "[{\"_id\":2,\"a\":2,\"b\":\"insertOne\"},{\"_id\":3,\"a\":3,\"b\":\"updateOne\"}]" );

   // not match docs
   var rc = cl.deleteOne( { "a": "notExist" } );
   assert.eq( rc, { "acknowledged": true, "deletedCount": 0 } );
   checkResults( cl.find(), "[{\"_id\":2,\"a\":2,\"b\":\"insertOne\"},{\"_id\":3,\"a\":3,\"b\":\"updateOne\"}]" );

   // all param
   var rc = cl.deleteOne( {}, { "writeConcern": { "j": true }, "collation": { "locale": "fr" } } );
   assert.eq( rc, { "acknowledged": true, "deletedCount": 1 } );
   checkResults( cl.find(), "[{\"_id\":3,\"a\":3,\"b\":\"updateOne\"}]" );


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