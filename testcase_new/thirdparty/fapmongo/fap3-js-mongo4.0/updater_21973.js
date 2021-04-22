/******************************************** 
@description : test updater
   note: when match multi docs, sdb update all match docs, mongo update one doc, not bug
@testcase    : seqDB-21973
@author      : XiaoNi Huang 2020-02-24
*********************************************/
main();

function main ()
{
   var clName = "cl21973";
   var cl = db.getCollection( clName );
   cl.drop();

   // $inc
   cl.insert( [{ "_id": 1, "a": 1, "b": 1 },
   { "_id": 2, "a": 2, "b": 1 },
   { "_id": 3, "a": 3, "b": 1 }] );
   var rc = cl.update( { "a": { "$in": [1, 2] } }, { "$inc": { "b": 1, "c": 1 } }, { "multi": true } );
   assert.eq( rc, { "nMatched": 2, "nUpserted": 0, "nModified": 2 } );
   checkResults( cl.find(), "[{\"_id\":1,\"a\":1,\"b\":2,\"c\":1},{\"_id\":2,\"a\":2,\"b\":2,\"c\":1},{\"_id\":3,\"a\":3,\"b\":1}]" );
   cl.remove( {} );


   // $set
   cl.insert( [{ "_id": 1, "a": 1, "b": 1 },
   { "_id": 2, "a": 2, "b": 1 },
   { "_id": 3, "a": 3, "b": 1 }] );
   var rc = cl.update( { "a": { "$in": [1, 2] } }, { "$set": { "b": "test", "c": 1 } }, { "multi": true } );
   assert.eq( rc, { "nMatched": 2, "nUpserted": 0, "nModified": 2 } );
   checkResults( cl.find(), "[{\"_id\":1,\"a\":1,\"b\":\"test\",\"c\":1},{\"_id\":2,\"a\":2,\"b\":\"test\",\"c\":1},{\"_id\":3,\"a\":3,\"b\":1}]" );
   cl.remove( {} );


   // $setOnInsert, doc not exists
   /*
   // upsert: false,
   // $setOnInsert only support upsert:true, return fail while upsert:false
   try
   {
      cl.update( { "a": 4 }, { "$setOnInsert": { "b": 4 } }, { "multi": true } );
      //throw new Error("expect fail but actual success.");  ---not throw error
      
      //actual return: WriteResult({ "writeError" : { "code" : -6, "errmsg" : "Invalid Argument" } })
      //expect throw error
   }
   catch( e )
   {
      assert.eq( e, 'Error: command failed: { "ok" : 0, "code" : -6, "errmsg" : "Invalid Argument" } : aggregate failed' );
   }
   
   */
   // upsert:true
   var rc = cl.update( { "a": 1 }, { "$setOnInsert": { "_id": 1, "a": 1 } }, { "upsert": true } );
   assert.eq( rc, { "nMatched": 0, "nUpserted": 1, "nModified": 0, "_id": 1 } );
   // check results
   var rc = cl.find();
   checkResults( rc, ["[{\"_id\":1,\"a\":1}]"] );
   cl.remove( {} );


   // $set + $inc + $setOnInsert
   // doc not exist, upsert:true, include _id
   var rc = cl.update( {}, { "$set": { "_id": 1 }, "$inc": { "a": 1 }, "$setOnInsert": { "b": 1 } }, { "upsert": true } );
   assert.eq( rc, { "nMatched": 0, "nUpserted": 1, "nModified": 0, "_id": 1 } );
   // check results
   var rc = cl.find();
   checkResults( rc, "[{\"_id\":1,\"a\":1,\"b\":1}]" );

   // doc not exist, upsert:true, not include _id
   var rc = cl.update( { "_id": 2 }, { "$set": { "a": 2 }, "$inc": { "b": 2 }, "$setOnInsert": { "c": 2 } }, { "upsert": true } );
   assert.eq( rc.nUpserted, 1 );
   // check results
   var rc = cl.find( {}, { "_id": 0, "a": 1, "b": 1, "c": 1 } );
   checkResults( rc, "[{\"a\":1,\"b\":1},{\"a\":2,\"b\":2,\"c\":2}]" );

   // doc exist, upsert:true
   var rc = cl.update( { "a": 1 }, { "$set": { "a": 3 }, "$inc": { "b": 3 }, "$setOnInsert": { "notExist": 3 } }, { "upsert": true } );
   assert.eq( rc, { "nMatched": 1, "nUpserted": 0, "nModified": 1 } );
   // check results
   var rc = cl.find( {} );
   checkResults( rc, "[{\"_id\":1,\"a\":3,\"b\":4},{\"_id\":2,\"a\":2,\"b\":2,\"c\":2}]" );
   cl.remove( {} );


   // $unset
   cl.insert( [{ "_id": 1, "a": 1, "b": 1 },
   { "_id": 2, "a": 2, "b": 2 },
   { "_id": 3, "a": 3, "b": 3 }] );
   var rc = cl.update( { "a": { "$lte": 2 } }, { "$unset": { "b": 1, "c": 1 } }, { "multi": true } );
   assert.eq( rc, { "nMatched": 2, "nUpserted": 0, "nModified": 2 } );
   checkResults( cl.find(), "[{\"_id\":1,\"a\":1},{\"_id\":2,\"a\":2},{\"_id\":3,\"a\":3,\"b\":3}]" );
   cl.remove( {} );


   // $pop: 1
   var docs = [{ "_id": 1, "a": 1, "b": [1, 2], "c": [1, 2, 3, 4, 5] },
   { "_id": 2, "a": 2, "b": [1, 2], "c": [1, 2, 3, 4, 5] },
   { "_id": 3, "a": 3, "b": [1, 2], "c": [1, 2, 3, 4, 5] }];
   cl.insert( docs );
   var rc = cl.update( { "a": { "$gt": 1 } }, { "$pop": { "b": 1, "c": 2 } }, { "multi": true } );
   assert.eq( rc, { "nMatched": 2, "nUpserted": 0, "nModified": 2 } );
   checkResults( cl.find(), "[{\"_id\":1,\"a\":1,\"b\":[1,2],\"c\":[1,2,3,4,5]},{\"_id\":2,\"a\":2,\"b\":[1],\"c\":[1,2,3]},{\"_id\":3,\"a\":3,\"b\":[1],\"c\":[1,2,3]}]" );

   // $pop: -1
   var rc = cl.update( { "a": { "$gt": 1 } }, { "$pop": { "b": -1, "c": -2 } }, { "multi": true } );
   assert.eq( rc, { "nMatched": 2, "nUpserted": 0, "nModified": 2 } );
   var expRC = "[{\"_id\":1,\"a\":1,\"b\":[1,2],\"c\":[1,2,3,4,5]},{\"_id\":2,\"a\":2,\"b\":[],\"c\":[3]},{\"_id\":3,\"a\":3,\"b\":[],\"c\":[3]}]";
   checkResults( cl.find(), expRC );

   // $pop: 0
   var rc = cl.update( { "a": 1 }, { "$pop": { "b": 0, "c": 0 } } );
   assert.eq( rc, { "nMatched": 1, "nUpserted": 0, "nModified": 0 } );
   checkResults( cl.find(), expRC );

   // field is empty array
   var rc = cl.update( { "a": 2 }, { "$pop": { "b": 1 } } );
   assert.eq( rc, { "nMatched": 1, "nUpserted": 0, "nModified": 1 } );
   checkResults( cl.find(), expRC );

   // field is not array
   var rc = cl.update( { "a": 1 }, { "$pop": { "a": 1 } } );
   assert.eq( rc, { "nMatched": 1, "nUpserted": 0, "nModified": 0 } );
   checkResults( cl.find(), expRC );

   // field is not exist
   var rc = cl.update( { "a": 1 }, { "$pop": { "notExistField": 1 } } );
   assert.eq( rc, { "nMatched": 1, "nUpserted": 0, "nModified": 0 } );
   checkResults( cl.find(), expRC );

   cl.remove( {} );


   // $pull
   var docs = [{ "_id": 1, "a": 1, "b": [], "c": [1, 2, 3, 4, 5] },
   { "_id": 2, "a": 2, "b": 1, "c": [1, 2, 3, 4, 5] },
   { "_id": 3, "a": 3, "b": [1], "c": [1, 3, 4, 5] }];
   cl.insert( docs );
   // filed exist, cover: is not array, empty array, value not exist 
   var rc = cl.update( {}, { "$pull": { "b": 1, "c": 2 } }, { "multi": true } );
   assert.eq( rc, { "nMatched": 3, "nUpserted": 0, "nModified": 3 } );
   var expRC = '[{\"_id\":1,\"a\":1,\"b\":[],\"c\":[1,3,4,5]},{\"_id\":2,\"a\":2,\"b\":1,\"c\":[1,3,4,5]},{\"_id\":3,\"a\":3,\"b\":[],\"c\":[1,3,4,5]}]';
   checkResults( cl.find(), expRC );

   // filed not exist
   var rc = cl.update( { "a": 3 }, { "$pull": { "notExistField": 2 } } );
   assert.eq( rc, { "nMatched": 1, "nUpserted": 0, "nModified": 0 } );
   checkResults( cl.find(), expRC );

   cl.remove( {} );


   // $push
   var docs = [{ "_id": 1, "a": 1, "b": [], "c": [1] },
   { "_id": 2, "a": 2, "b": 1, "c": [1] },
   { "_id": 3, "a": 3, "b": [1], "c": [1] }];
   cl.insert( docs );
   // filed exist, cover: is not array, empty array, value not exist 
   var rc = cl.update( {}, { "$push": { "b": 1, "c": 2 } }, { "multi": true } );
   assert.eq( rc, { "nMatched": 3, "nUpserted": 0, "nModified": 3 } );
   var expRC = "[{\"_id\":1,\"a\":1,\"b\":[1],\"c\":[1,2]},{\"_id\":2,\"a\":2,\"b\":1,\"c\":[1,2]},{\"_id\":3,\"a\":3,\"b\":[1,1],\"c\":[1,2]}]";
   checkResults( cl.find(), expRC );

   // filed not exist
   var rc = cl.update( { "a": 1 }, { "$pull": { "notExistField": 3 } } );
   assert.eq( rc, { "nMatched": 1, "nUpserted": 0, "nModified": 0 } );
   checkResults( cl.find(), expRC );
   cl.remove( {} );


   // others
   // doc not exist, upsert:true, matcher field not same updater field
   var rc = cl.update( { "a": 1 }, { "$set": { "_id": 1 }, "$inc": { "b": 1 } }, { "upsert": true } );
   assert.eq( rc, { "nMatched": 0, "nUpserted": 1, "nModified": 0, "_id": 1 } );
   // doc not exist, upsert:false
   var rc = cl.update( { "a": 2 }, { "$set": { "_id": 2 }, "$inc": { "b": 2 } }, { "upsert": false } );
   assert.eq( rc, { "nMatched": 0, "nUpserted": 0, "nModified": 0 } );
   // check results
   checkResults( cl.find(), "[{\"_id\":1,\"a\":1,\"b\":1}]" );


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