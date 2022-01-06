/******************************************************************************
 * @Description   : seqDB-24160:bulkWrite执行单个操作
 * @Author        : XiaoNi Huang
 * @CreateTime    : 2021.04.22
 * @LastEditTime  : 2021.05.17
 * @LastEditors   : XiaoNi Huang
 ******************************************************************************/
main();

function main ()
{
   var clName = "mongod_cl_24160";
   var cl = db.getCollection( clName );
   cl.drop();

   cl.bulkWrite( [{ "insertOne": { "document": { 'init': 1 } } }] );
   cl.bulkWrite( [{ "deleteMany": { "filter": {} } }] );
   assert.eq( cl.count(), 0 );

   bulkWriteInsertOne( cl );
   bulkWriteUpdateOne( cl );
   bulkWriteUpdateMany( cl );
   bulkWriteReplaceOne( cl );
   bulkWriteDeleteOne( cl );
   bulkWriteDeleteMany( cl );

   cl.drop();
}

function bulkWriteInsertOne ( cl )
{
   // clear docs
   cl.bulkWrite( [{ "deleteMany": { "filter": {} } }] );
   // insert_one
   var rc = cl.bulkWrite( [{ "insertOne": { "document": { '_id': 1, 'a': 1 } } }] );
   checkBulkWriteReturn( rc, 1, { "0": 1 }, 0, 0, 0, {} );
   // insert_one again
   var rc = cl.bulkWrite( [{ "insertOne": { "document": { '_id': 2, 'a': 2 } } }] );
   checkBulkWriteReturn( rc, 1, { "0": 2 }, 0, 0, 0, {} );
   // check docs
   var cursor = cl.find().sort( { '_id': 1 } );
   var expDocs = [{ '_id': 1, 'a': 1 }, { '_id': 2, 'a': 2 }];
   checkResults( cursor, JSON.stringify( expDocs ) );
}

function bulkWriteUpdateOne ( cl )
{
   // ready data
   cl.bulkWrite( [{ "deleteMany": { "filter": {} } }] );
   cl.bulkWrite( [{ "insertOne": { "document": { '_id': 1, 'a': 1, 'b': 1 } } }] );
   cl.bulkWrite( [{ "insertOne": { "document": { '_id': 2, 'a': 2, 'b': 1 } } }] );

   // test1: updateOne, match docs, upsert is functionault
   var rc = cl.bulkWrite( [{ "updateOne": { "filter": {}, "update": { '$set': { 'b': 2 } } } }] );
   checkBulkWriteReturn( rc, 0, {}, 1, 0, 0, {} );
   // check docs
   var cursor = cl.find().sort( { '_id': 1 } );
   var expDocs = [{ '_id': 1, 'a': 1, 'b': 2 }, { '_id': 2, 'a': 2, 'b': 1 }];
   checkResults( cursor, JSON.stringify( expDocs ) );

   // test2: updateOne, not match docs of normal field, upsert :true
   var rc = cl.bulkWrite( [{ "updateOne": { "filter": { 'c': 1 }, "update": { '$set': { '_id': 3, 'a': 3 } }, "upsert": true } }] );
   checkBulkWriteReturn( rc, 0, {}, 0, 0, 1, { "0": 3 } );
   // check docs
   var cursor = cl.find().sort( { '_id': 1 } );
   var expDocs = [{ '_id': 1, 'a': 1, 'b': 2 }, { '_id': 2, 'a': 2, 'b': 1 },
   { '_id': 3, 'a': 3, 'c': 1 }];
   checkResults( cursor, JSON.stringify( expDocs ) );

   // test3: updateOne, not match docs of _id field, upsert :true
   var rc = cl.bulkWrite( [{ "updateOne": { "filter": { '_id': 4 }, "update": { '$set': { 'a': 4 } }, "upsert": true } }] );
   checkBulkWriteReturn( rc, 0, {}, 0, 0, 1, { "0": 4 } );
   // check docs
   var cursor = cl.find().sort( { '_id': 1 } );
   var expDocs = [{ '_id': 1, 'a': 1, 'b': 2 }, { '_id': 2, 'a': 2, 'b': 1 },
   { '_id': 3, 'a': 3, 'c': 1 }, { '_id': 4, 'a': 4 }];
   checkResults( cursor, JSON.stringify( expDocs ) );
}

function bulkWriteUpdateMany ( cl )
{
   // ready data
   cl.bulkWrite( [{ "deleteMany": { "filter": {} } }] );
   cl.bulkWrite( [{ "insertOne": { "document": { '_id': 1, 'a': 1, 'b': 1 } } }] );
   cl.bulkWrite( [{ "insertOne": { "document": { '_id': 2, 'a': 2, 'b': 1 } } }] );

   // test1: updateOne, match docs, upsert is functionault
   var rc = cl.bulkWrite( [{ "updateMany": { "filter": {}, "update": { '$inc': { 'a': 1 }, '$set': { 'b': 2 } } } }] );
   checkBulkWriteReturn( rc, 0, {}, 2, 0, 0, {} );
   // check docs
   var cursor = cl.find().sort( { '_id': 1 } );
   var expDocs = [{ '_id': 1, 'a': 2, 'b': 2 }, { '_id': 2, 'a': 3, 'b': 2 }];
   checkResults( cursor, JSON.stringify( expDocs ) );

   // test2: UpdateMany, not match docs of normal field, upsert :true
   var rc = cl.bulkWrite( [{ "updateMany": { "filter": { 'c': 1 }, "update": { '$set': { '_id': 3, 'a': 3 } }, "upsert": true } }] );
   checkBulkWriteReturn( rc, 0, {}, 0, 0, 1, { "0": 3 } );
   // check docs
   var cursor = cl.find().sort( { '_id': 1 } );
   var expDocs = [{ '_id': 1, 'a': 2, 'b': 2 }, { '_id': 2, 'a': 3, 'b': 2 },
   { '_id': 3, 'a': 3, 'c': 1 }];
   checkResults( cursor, JSON.stringify( expDocs ) );

   // test3: UpdateMany, not match docs of _id field, upsert :true
   var rc = cl.bulkWrite( [{ "updateMany": { "filter": { '_id': 4 }, "update": { '$set': { 'a': 4 } }, "upsert": true } }] );
   checkBulkWriteReturn( rc, 0, {}, 0, 0, 1, { "0": 4 } );
   // check docs
   var cursor = cl.find().sort( { '_id': 1 } );
   var expDocs = [{ '_id': 1, 'a': 2, 'b': 2 }, { '_id': 2, 'a': 3, 'b': 2 },
   { '_id': 3, 'a': 3, 'c': 1 }, { '_id': 4, 'a': 4 }];
   checkResults( cursor, JSON.stringify( expDocs ) );
}

function bulkWriteReplaceOne ( cl )
{
   // ready data
   cl.bulkWrite( [{ "deleteMany": { "filter": {} } }] );
   cl.bulkWrite( [{ "insertOne": { "document": { '_id': 1, 'a': 1, 'b': 1 } } }] );
   cl.bulkWrite( [{ "insertOne": { "document": { '_id': 2, 'a': 2, 'b': 1 } } }] );

   // test1: updateOne, match docs, upsert is functionault
   var rc = cl.bulkWrite( [{ "replaceOne": { "filter": { 'b': 1 }, "replacement": { 'c': 1 } } }] );
   checkBulkWriteReturn( rc, 0, {}, 1, 0, 0, {} );
   // check docs
   var cursor = cl.find().sort( { '_id': 1 } );
   var expDocs = [{ '_id': 1, 'c': 1 }, { '_id': 2, 'a': 2, 'b': 1 }];
   checkResults( cursor, JSON.stringify( expDocs ) );

   // test2: UpdateMany, not match docs of normal field, upsert :true
   var rc = cl.bulkWrite( [{ "replaceOne": { "filter": { 'd': 1 }, "replacement": { '_id': 3, 'e': 1 }, "upsert": true } }] );
   checkBulkWriteReturn( rc, 0, {}, 0, 0, 1, { "0": 3 } );
   // check docs
   var cursor = cl.find().sort( { '_id': 1 } )
   var expDocs = [{ '_id': 1, 'c': 1 }, { '_id': 2, 'a': 2, 'b': 1 }, { '_id': 3, 'e': 1 }]
   checkResults( cursor, JSON.stringify( expDocs ) )
}

function bulkWriteDeleteOne ( cl )
{
   // ready data
   cl.bulkWrite( [{ "deleteMany": { "filter": {} } }] );
   cl.bulkWrite( [{ "insertOne": { "document": { '_id': 1, 'a': 1, 'b': 1 } } }] );
   cl.bulkWrite( [{ "insertOne": { "document": { '_id': 2, 'a': 2, 'b': 1 } } }] );
   cl.bulkWrite( [{ "insertOne": { "document": { '_id': 3, 'a': 3, 'b': 1 } } }] );

   // test1: matcher = {}
   var rc = cl.bulkWrite( [{ "deleteOne": { "filter": {} } }] );
   checkBulkWriteReturn( rc, 0, {}, 0, 1, 0, {} );
   // check docs
   var cursor = cl.find().sort( { '_id': 1 } );
   var expDocs = [{ '_id': 2, 'a': 2, 'b': 1 }, { '_id': 3, 'a': 3, 'b': 1 }];
   checkResults( cursor, JSON.stringify( expDocs ) );

   // test2: matcher = { ...}, multi docs
   var rc = cl.bulkWrite( [{ "deleteOne": { "filter": { 'b': 1 } } }] );
   checkBulkWriteReturn( rc, 0, {}, 0, 1, 0, {} );
   // check docs
   var cursor = cl.find().sort( { '_id': 1 } );
   var expDocs = [{ '_id': 3, 'a': 3, 'b': 1 }];
   checkResults( cursor, JSON.stringify( expDocs ) );
}

function bulkWriteDeleteMany ( cl )
{
   // ready data
   cl.bulkWrite( [{ "deleteMany": { "filter": {} } }] );
   for( i = 0; i < 10; i++ )
   {
      cl.bulkWrite( [{ "insertOne": { "document": { '_id': i, 'a': i, 'b': 1 } } }] );
   }

   // test1: matcher = { ...}
   var rc = cl.bulkWrite( [{ "deleteMany": { "filter": { 'a': { '$gte': 5 } } } }] );
   checkBulkWriteReturn( rc, 0, {}, 0, 5, 0, {} );
   assert.eq( cl.count(), 5 );

   // test2: matcher = {}
   var rc = cl.bulkWrite( [{ "deleteMany": { "filter": {} } }] );
   checkBulkWriteReturn( rc, 0, {}, 0, 5, 0, {} );
   assert.eq( cl.count(), 0 );
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
