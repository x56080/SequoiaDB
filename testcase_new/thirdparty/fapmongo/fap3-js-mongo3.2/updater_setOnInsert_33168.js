/******************************************************************************
 * @Description   : seqDB-33168:findAndModify 支持 setOnInsert
 * @Author        : XiaoNi Huang
 * @CreateTime    : 2023.9.5
 * @LastEditTime  : 2023.9.5
 * @LastEditors   : XiaoNi Huang
 ******************************************************************************/

main();
function main ()
{
   var clName = "cl_33168";
   var cl = db.getCollection( clName );
   cl.drop();
   cl.insertMany( [{ '_id': 1, 'a': 1 }] );

   // findAndModify
   // 匹配不存在的记录，"upsert": false
   var rc = cl.findAndModify( { "query": { "_id": 2 }, "update": { "$setOnInsert": { "a": 2 } }, "upsert": false, "new": true } );
   assert.eq( rc, null );
   // 匹配存在的记录，"upsert": true，$setOnInsert 不存在的字段
   var rc = cl.findAndModify( { "query": { "_id": 1 }, "update": { "$setOnInsert": { "b": 2 } }, "upsert": false, "new": true } );
   assert.eq( rc, { "_id": 1, "a": 1 } );
   // 匹配不存在的记录，"upsert": true
   var rc = cl.findAndModify( { "query": { "_id": 3 }, "update": { "$setOnInsert": { "a": 3 } }, "upsert": true, "new": true } );
   assert.eq( rc, { "_id": 3, "a": 3 } );
   // 检查结果
   var rc = cl.find().sort( { "_id": 1 } );
   checkResults( rc, "[{\"_id\":1,\"a\":1},{\"_id\":3,\"a\":3}]" );


   // updateOne
   var rc = cl.updateOne( { "_id": 4 }, { "$setOnInsert": { "a": 4 } }, { "upsert": true } );
   assert.eq( rc, { "acknowledged": true, "matchedCount": 0, "modifiedCount": 0, "upsertedId": 4 } );
   // 检查结果
   var rc = cl.find().sort( { "_id": 1 } );
   checkResults( rc, "[{\"_id\":1,\"a\":1},{\"_id\":3,\"a\":3},{\"_id\":4,\"a\":4}]" );


   // cl.updateMany
   var rc = cl.updateMany( { "_id": 5 }, { "$setOnInsert": { "a": 4 } }, { "upsert": true } );
   assert.eq( rc, { "acknowledged": true, "matchedCount": 0, "modifiedCount": 0, "upsertedId": 5 } );
   // 检查结果
   var rc = cl.find().sort( { "_id": 1 } );
   checkResults( rc, "[{\"_id\":1,\"a\":1},{\"_id\":3,\"a\":3},{\"_id\":4,\"a\":4},{\"_id\":5,\"a\":4}]" );

   cl.drop();
}

function checkResults ( cursor, expDocs )
{
   var docs = new Array();
   while( cursor.hasNext() )
   {
      var doc = cursor.next();
      docs.push( doc );
   }
   cursor.close();
   assert.eq( JSON.stringify( docs ), expDocs );
}