/******************************************** 
@description : insert/update/find/remove
@testcase    : seqDB-21963
@author      : XiaoNi Huang 2020-02-24
*********************************************/
main();

function main ()
{
   var clName = "cl21963";
   var cl = db.getCollection( clName );
   cl.drop();

   // ready data
   var docs = [{ "_id": 1, "a": 1, "b": "insert" },
   { "_id": 2, "a": 2, "b": "insert" },
   { "_id": 3, "a": 3, "b": "insert" },
   { "_id": 4, "a": 4, "b": "insert" },
   { "_id": 5, "a": 5, "b": "insert" }];

   // cl.insert(<document or array documents>,{writeConcern:<document>,ordered:<boolean>})
   // insert document
   var rc = cl.insert( docs[0] );
   assert.eq( rc, { "nInserted": 1 } );
   var rc = db.getLastError();
   assert.eq( rc, null );

   // insert []
   var rc = cl.insert( [] );
   assert.eq( rc, { "writeErrors": [], "writeConcernErrors": [], "nInserted": 0, "nUpserted": 0, "nMatched": 0, "nModified": 0, "nRemoved": 0, "upserted": [] } );

   // insert [documents]
   var rc = cl.insert( [docs[1], docs[2]] );
   assert.eq( rc, { "writeErrors": [], "writeConcernErrors": [], "nInserted": 2, "nUpserted": 0, "nMatched": 0, "nModified": 0, "nRemoved": 0, "upserted": [] } );

   // insert [documents], point writeConcern and ordered
   var rc = cl.insert( [docs[4], docs[3]], { "writeConcern": { "w": 1 }, "ordered": true } );
   assert.eq( rc, { "writeErrors": [], "writeConcernErrors": [], "nInserted": 2, "nUpserted": 0, "nMatched": 0, "nModified": 0, "nRemoved": 0, "upserted": [] } );

   // check results
   var rc = cl.find().sort( { "a": 1 } );
   checkResults( rc, JSON.stringify( docs ) );


   // cl.update(<filter>,<update>,{upsert:<boolean>,multi:<boolean>,writeConcern:<document>,collation:<document>})
   // cl.update(<filter>,<update>), multi default: false
   var rc = cl.update( { "a": { "$lt": 5 } }, { "$set": { "b": "test" } } );
   assert.eq( rc, { "nMatched": 1, "nUpserted": 0, "nModified": 1 } );

   // cl.update(<filter>,<update>, {multi:true})
   var rc = cl.update( { "a": { "$exists": 1 } }, { "$set": { "u2": 1 } }, { "multi": true } );
   assert.eq( rc, { "nMatched": 5, "nUpserted": 0, "nModified": 5 } );

   // cl.update(<filter>,<update>, {writeConcern:<document>,collation:<document>})
   var rc = cl.update( { "_id": 2 }, { "$set": { "u3": 1 } }, { "writeConcern": { "j": true }, "collation": { "locale": "fr" } } );
   assert.eq( rc, { "nMatched": 1, "nUpserted": 0, "nModified": 1 } );

   // cl.update(<filter>,<update>}), filter empty
   var rc = cl.update( {}, { "$set": { "u4": 1 } } );
   assert.eq( rc, { "nMatched": 1, "nUpserted": 0, "nModified": 1 } );
   // db.getLastError()
   var rc = db.getLastError();
   assert.eq( rc, null );


   // cl.update(<filter>,<update>}), filter empty, multi:true
   var rc = cl.update( {}, { "b": "hello" }, { "multi": true } );
   if( rc.toString().indexOf( "multi update is not supported for replacement-style update" ) === -1 )
   {
      throw new Error( "check fail, rc: " + rc.toString() );
   }
   var rc = db.getLastError();
   assert.eq( rc, "multi update is not supported for replacement-style update" );

   // check results
   var rc = cl.find().sort( { "a": 1 } );
   var expDocs = "[{\"_id\":1,\"a\":1,\"b\":\"test\",\"u2\":1,\"u4\":1},{\"_id\":2,\"a\":2,\"b\":\"insert\",\"u2\":1,\"u3\":1},{\"_id\":3,\"a\":3,\"b\":\"insert\",\"u2\":1},{\"_id\":4,\"a\":4,\"b\":\"insert\",\"u2\":1},{\"_id\":5,\"a\":5,\"b\":\"insert\",\"u2\":1}]";
   checkResults( rc, expDocs );


   // cl.remove
   // remove part docs
   var rc = cl.remove( { "_id": { "$lt": 4 } } );
   assert.eq( rc, { "nRemoved": 3 } );
   var rc = cl.find().sort( { "a": 1 } );
   var expDocs = "[{\"_id\":4,\"a\":4,\"b\":\"insert\",\"u2\":1},{\"_id\":5,\"a\":5,\"b\":\"insert\",\"u2\":1}]";
   checkResults( rc, expDocs );

   // remove not exist doc
   var rc = cl.remove( { "notExist": 1 } );
   assert.eq( rc, { "nRemoved": 0 } );
   var rc = cl.find().sort( { "a": 1 } );
   checkResults( rc, expDocs );

   // remove({})
   var cnt = cl.count();
   var rc = cl.remove( {} );
   assert.eq( rc, { "nRemoved": cnt } );
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