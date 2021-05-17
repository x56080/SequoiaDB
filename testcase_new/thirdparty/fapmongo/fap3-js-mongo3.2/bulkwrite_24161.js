/******************************************************************************
 * @Description   : seqDB-24161:bulkWrite批量执行多个操作bulkWrite
 * @Author        : XiaoNi Huang
 * @CreateTime    : 2021.04.22
 * @LastEditTime  : 2021.05.07
 * @LastEditors   : XiaoNi Huang
 ******************************************************************************/
main();

function main ()
{
   var clName = "mongod_cl_24161";
   var cl = db.getCollection( clName );
   cl.drop();

   cl.bulkWrite( [{ "insertOne": { "document": { 'init': 1 } } }] );
   cl.bulkWrite( [{ "deleteMany": { "filter": {} } }] );
   assert.eq( cl.count(), 0 );

   bulkWrite01( cl );
   bulkWrite02( cl );
   bulkWrite03( cl );
   bulkWrite04( cl );
   bulkWrite05( cl );

   cl.drop();
}

function bulkWrite01 ( cl )
{
   // ready data
   readyData( cl );

   // test: insertOne / updateOne / updateMany / replaceOne / deleteMany / deleteOne
   var rc = cl.bulkWrite( [
      { "insertOne": { "document": { '_id': 11, 'a': 11, 'b': 1 } } },
      { "updateOne": { "filter": { 'a': { '$lt': 2 } }, "update": { '$set': { 'b': 2 } } } },
      { "updateMany": { "filter": { "$and": [{ "a": { "$gte": 2 } }, { "a": { "$lt": 4 } }] }, "update": { '$set': { 'b': 2 } } } },
      { "replaceOne": { "filter": { 'a': 5 }, "replacement": { 'a': 5, 'd': 2 } } },
      { "deleteOne": { "filter": { "$and": [{ "a": { "$gte": 6 } }, { "a": { "$lt": 8 } }] } } },
      { "deleteMany": { "filter": { "$and": [{ "a": { "$gte": 8 } }, { "a": { "$lt": 10 } }] } } }] );
   checkBulkWriteReturn( rc, 1, { "0": 11 }, 4, 3, 0, {} );
   // check docs
   assert.eq( cl.count(), 8 );
   var cursor = cl.find().sort( { '_id': 1 } );
   var expDocs = [
      { '_id': 0, 'a': 0, 'b': 2 },
      { '_id': 1, 'a': 1, 'b': 1 },
      { '_id': 2, 'a': 2, 'b': 2 },
      { '_id': 3, 'a': 3, 'b': 2 },
      { '_id': 4, 'a': 4, 'b': 1 },
      { '_id': 5, 'a': 5, 'd': 2 },
      { '_id': 7, 'a': 7, 'b': 1 },
      { '_id': 11, 'a': 11, 'b': 1 }];
   checkResults( cursor, JSON.stringify( expDocs ) );
}

function bulkWrite02 ( cl )
{
   // ready data
   readyData( cl );

   // test: insertOne / updateOne["upsert" :true] / updateMany["upsert" :true] /
   // replaceOne["upsert" :true] / deleteMany / deleteOne
   requests = [
      { "insertOne": { "document": { '_id': 11, 'a': 11, 'b': 1 } } },
      { "updateOne": { "filter": { 'updateOne': 1 }, "update": { '$set': { '_id': 12, 'updateOne2': 1 } }, "upsert": true } },
      { "updateMany": { "filter": { 'updateMany': 1 }, "update": { '$set': { '_id': 13, 'updateMany2': 1 } }, "upsert": true } },
      { "replaceOne": { "filter": { 'replaceOne': 1 }, "replacement": { '_id': 14, 'replaceOne2': 2 }, "upsert": true } },
      { "deleteOne": { "filter": { "$and": [{ "a": { "$gte": 4 } }, { "a": { "$lt": 6 } }] } } },
      { "deleteMany": { "filter": { "$and": [{ "a": { "$gte": 6 } }, { "a": { "$lt": 8 } }] } } }]
   var rc = cl.bulkWrite( requests );
   checkBulkWriteReturn( rc, 1, { "0": 11 }, 0, 3, 3, { "1": 12, "2": 13, "3": 14 } );
   // check docs
   assert.eq( cl.count(), 11 );
   var cursor = cl.find().sort( { '_id': 1 } );
   var expDocs = [
      { '_id': 0, 'a': 0, 'b': 1 },
      { '_id': 1, 'a': 1, 'b': 1 },
      { '_id': 2, 'a': 2, 'b': 1 },
      { '_id': 3, 'a': 3, 'b': 1 },
      { '_id': 5, 'a': 5, 'b': 1 },
      { '_id': 8, 'a': 8, 'b': 1 },
      { '_id': 9, 'a': 9, 'b': 1 },
      { '_id': 11, 'a': 11, 'b': 1 },
      { 'updateOne': 1, 'updateOne2': 1, '_id': 12 },
      { 'updateMany': 1, 'updateMany2': 1, '_id': 13 },
      { 'replaceOne2': 2, '_id': 14 }];
   checkResults( cursor, JSON.stringify( expDocs ) );
}

function bulkWrite03 ( cl )
{
   // ready data
   readyData( cl )

   // test: the same multiple operations.
   requests = [
      { "insertOne": { "document": { '_id': 11, 'a': 11, 'b': 1 } } },
      { "insertOne": { "document": { '_id': 12, 'a': 12, 'b': 1 } } },
      { "insertOne": { "document": { '_id': 13, 'a': 13, 'b': 1 } } },

      { "updateOne": { "filter": { 'a': 1 }, "update": { '$set': { 'b': 2 } } } },
      { "updateOne": { "filter": { 'a': 2 }, "update": { '$set': { 'updateOne1': 1 } } } },
      { "updateOne": { "filter": { 'updateOne': 1 }, "update": { '$set': { '_id': 14, 'updateOne2': 1 } } } },

      { "updateMany": { "filter": { "$and": [{ "a": { "$gte": 3 } }, { "a": { "$lt": 5 } }] }, "update": { '$set': { 'b': 3 } } } },
      { "updateMany": { "filter": { "$and": [{ "a": { "$gte": 5 } }, { "a": { "$lt": 7 } }] }, "update": { '$set': { 'updateMany1': 1 } } } },
      { "updateMany": { "filter": { 'updateMany': 1 }, "update": { '$set': { '_id': 15, 'updateMany2': 1 } } } },

      { "replaceOne": { "filter": { 'c': 1 }, "replacement": { 'd': 2 } } },
      { "replaceOne": { "filter": { 'replaceOne': 1 }, "replacement": { '_id': 16, 'replaceOne2': 2 } } },

      { "deleteOne": { "filter": { 'a': 7 } } },
      { "deleteMany": { "filter": { "$and": [{ "a": { "$gte": 8 } }, { "a": { "$lt": 10 } }] } } }];
   var rc = cl.bulkWrite( requests );
   checkBulkWriteReturn( rc, 3, { "0": 11, "1": 12, "2": 13 }, 6, 3, 0, {} );
   // check docs
   assert.eq( cl.count(), 10 );
   var cursor = cl.find().sort( { '_id': 1 } );
   var expDocs = [
      { '_id': 0, 'a': 0, 'b': 1 },
      { '_id': 1, 'a': 1, 'b': 2 },
      { 'updateOne1': 1, '_id': 2, 'a': 2, 'b': 1 },
      { '_id': 3, 'a': 3, 'b': 3 },
      { '_id': 4, 'a': 4, 'b': 3 },
      { 'updateMany1': 1, '_id': 5, 'a': 5, 'b': 1 },
      { 'updateMany1': 1, '_id': 6, 'a': 6, 'b': 1 },
      { '_id': 11, 'a': 11, 'b': 1 },
      { '_id': 12, 'a': 12, 'b': 1 },
      { '_id': 13, 'a': 13, 'b': 1 }];
   checkResults( cursor, JSON.stringify( expDocs ) );
}

function bulkWrite04 ( cl )
{
   // ready data
   readyData( cl )

   // test: the same multiple operations. upsert:true
   requests = [
      { "insertOne": { "document": { '_id': 11, 'a': 11, 'b': 1 } } },
      { "insertOne": { "document": { '_id': 12, 'a': 12, 'b': 1 } } },
      { "insertOne": { "document": { '_id': 13, 'a': 13, 'b': 1 } } },

      { "updateOne": { "filter": { 'a': 1 }, "update": { '$set': { 'b': 2 } } } },
      { "updateOne": { "filter": { 'a': 2 }, "update": { '$set': { 'updateOne1': 1 } } } },
      { "updateOne": { "filter": { 'updateOne': 1 }, "update": { '$set': { '_id': 14, 'updateOne2': 1 } }, "upsert": true } },

      { "updateMany": { "filter": { "$and": [{ "a": { "$gte": 3 } }, { "a": { "$lt": 5 } }] }, "update": { '$set': { 'b': 3 } } } },
      { "updateMany": { "filter": { "$and": [{ "a": { "$gte": 5 } }, { "a": { "$lt": 7 } }] }, "update": { '$set': { 'updateMany1': 1 } } } },
      { "updateMany": { "filter": { 'updateMany': 1 }, "update": { '$set': { '_id': 15, 'updateMany2': 1 } }, "upsert": true } },

      { "replaceOne": { "filter": { 'c': 1 }, "replacement": { 'd': 2 } } },
      { "replaceOne": { "filter": { 'replaceOne': 1 }, "replacement": { '_id': 16, 'replaceOne2': 2 }, "upsert": true } },

      { "deleteOne": { "filter": { 'a': 7 } } },
      { "deleteMany": { "filter": { "$and": [{ "a": { "$gte": 8 } }, { "a": { "$lt": 10 } }] } } }];
   var rc = cl.bulkWrite( requests );
   checkBulkWriteReturn( rc, 3, { "0": 11, "1": 12, "2": 13 }, 6, 3, 3, { 8: 15, 10: 16, 5: 14 } );
   // check docs
   assert.eq( cl.count(), 13 );
   var cursor = cl.find().sort( { '_id': 1 } );
   var expDocs = [
      { '_id': 0, 'a': 0, 'b': 1 },
      { '_id': 1, 'a': 1, 'b': 2 },
      { 'updateOne1': 1, '_id': 2, 'a': 2, 'b': 1 },
      { '_id': 3, 'a': 3, 'b': 3 },
      { '_id': 4, 'a': 4, 'b': 3 },
      { 'updateMany1': 1, '_id': 5, 'a': 5, 'b': 1 },
      { 'updateMany1': 1, '_id': 6, 'a': 6, 'b': 1 },
      { '_id': 11, 'a': 11, 'b': 1 },
      { '_id': 12, 'a': 12, 'b': 1 },
      { '_id': 13, 'a': 13, 'b': 1 },
      { 'updateOne': 1, 'updateOne2': 1, '_id': 14 },
      { 'updateMany': 1, 'updateMany2': 1, '_id': 15 },
      { 'replaceOne2': 2, '_id': 16 }];
   checkResults( cursor, JSON.stringify( expDocs ) );
}

function bulkWrite05 ( cl )
{
   // ready data
   readyData( cl );

   // test: the same multiple operations. operation failed
   var requests = [
      { "insertOne": { "document": { '_id': 11, 'a': 11, 'b': 1 } } },

      { "updateOne": { "filter": { 'a': 1 }, "update": { '$error': { 'b': 2 } }, "upsert": true } },

      { "updateMany": { "filter": { "$and": [{ "a": { "$gte": 3 } }, { "a": { "$lt": 5 } }] }, "update": { '$set': { 'b': 3 } } } }];
   try
   {
      cl.bulkWrite( requests );
      throw new Error( "expect fail but actual success." );
   }
   catch( e )
   {
      assert.eq( e.code, -6 );
   }
   // check docs
   assert.eq( cl.count(), 11 );
   var cursor = cl.find().sort( { '_id': 1 } );
   var expDocs = [
      { '_id': 0, 'a': 0, 'b': 1 },
      { '_id': 1, 'a': 1, 'b': 1 },
      { '_id': 2, 'a': 2, 'b': 1 },
      { '_id': 3, 'a': 3, 'b': 1 },
      { '_id': 4, 'a': 4, 'b': 1 },
      { '_id': 5, 'a': 5, 'b': 1 },
      { '_id': 6, 'a': 6, 'b': 1 },
      { '_id': 7, 'a': 7, 'b': 1 },
      { '_id': 8, 'a': 8, 'b': 1 },
      { '_id': 9, 'a': 9, 'b': 1 },
      { '_id': 11, 'a': 11, 'b': 1 }]
   checkResults( cursor, JSON.stringify( expDocs ) );
}

function readyData ( cl )
{
   cl.bulkWrite( [{ "deleteMany": { "filter": {} } }] );
   for( i = 0; i < 10; i++ )
   {
      cl.bulkWrite( [{ "insertOne": { "document": { '_id': i, 'a': i, 'b': 1 } } }] );
   }
}

function checkBulkWriteReturn ( rc, insertedCount, insertedIds, matchedCount, deletedCount, upsertedCount, upsertedIds )
{
   assert.eq( rc.insertedCount, insertedCount, JSON.stringify( rc ) );
   assert.eq( rc.insertedIds, insertedIds, JSON.stringify( rc ) );
   assert.eq( rc.matchedCount, matchedCount, JSON.stringify( rc ) );
   assert.eq( rc.deletedCount, deletedCount, JSON.stringify( rc ) );
   assert.eq( rc.upsertedCount, upsertedCount, JSON.stringify( rc ) );
   assert.eq( rc.upsertedIds, upsertedIds, JSON.stringify( rc ) );
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