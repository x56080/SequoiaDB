/******************************************************************************
 * @Description   : seqDB - 24147: findOneAndUpdate / findOneAndReplace / findOneAndDelete操作
 * @Author        : XiaoNi Huang
 * @CreateTime    : 2021.04.15
 * @LastEditTime  : 2021.05.19
 * @LastEditors   : XiaoNi Huang
 ******************************************************************************/
main();

function main ()
{
   var clName = "fapmongo_cl_24147";
   var cl = db.getCollection( clName );
   cl.drop();
   cl.createIndex( { "a": 1 } );

   testFindOneAndUpdate( cl );
   testFindOneAndUpdateWithUpsert( cl );
   testFindOneAndUpdateWithReturnNewDocument( cl );
   testFindOneAndUpdateWithSort( cl );

   testFindOneAndReplace( cl );
   testFindOneAndReplaceWithUpsert( cl );
   testFindOneAndReplaceWithReturnNewDocument( cl );
   testFindOneAndReplaceWithSort( cl );

   testFindOneAndDelete( cl );
   testFindOneAndDeleteWithSort( cl );

   testNotExistDB_findOneAndXXX( cl );
   testNotExistCL_findOneAndXXX( cl );

   cl.drop();
}

function testFindOneAndUpdate ( cl )
{
   cl.deleteMany( {} );
   cl.insertMany( [{ "_id": 1, "a": 1 }, { "_id": 2, "a": 2 }] );

   // 匹配存在的记录，更新
   var rc = cl.findOneAndUpdate( { 'a': { '$gt': 0 } }, { '$inc': { 'a': 10 } } );
   assert.eq( rc, { "_id": 1, "a": 1 } );

   // 匹配不存在的记录，更新
   var rc = cl.findOneAndUpdate( { 'a': 3 }, { '$set': { '_id': 3, 'b': 3 } } );
   assert.eq( rc, null );

   // 检查结果
   var rc = cl.find().sort( { "_id": 1 } );
   checkResults( rc, "[{\"_id\":1,\"a\":11},{\"_id\":2,\"a\":2}]" );
}

function testFindOneAndUpdateWithUpsert ( cl )
{
   cl.deleteMany( {} );

   // 匹配不存在的记录
   // { "upsert": true }
   var rc = cl.findOneAndUpdate( { 'a': 1 }, { '$set': { '_id': 1, 'a': 1 } }, { "upsert": true } );
   assert.eq( rc, null );
   // { "upsert": false }
   var rc = cl.findOneAndUpdate( { 'a': 2 }, { '$set': { '_id': 2, 'a': 2 } }, { "upsert": false } );
   assert.eq( rc, null );
   // 检查结果
   var rc = cl.find().sort( { "_id": 1 } );
   checkResults( rc, "[{\"_id\":1,\"a\":1}]" );

   // 匹配存在的记录，{ "upsert": true }
   var rc = cl.findOneAndUpdate( { 'a': 1 }, { '$set': { 'b': 1 } }, { "upsert": true } );
   assert.eq( rc, { "_id": 1, "a": 1 } );
   // 检查结果
   var rc = cl.find().sort( { "_id": 1 } );
   checkResults( rc, "[{\"_id\":1,\"a\":1,\"b\":1}]" );
}

function testFindOneAndUpdateWithReturnNewDocument ( cl )
{
   // returnNewDocument，True更新或插入后的文档，False返回原始文档
   cl.deleteMany( {} );
   cl.insertMany( [{ '_id': 1, 'a': 1 }, { '_id': 2, 'a': 2 }] );

   // update，{"returnNewDocument": true}
   var rc = cl.findOneAndUpdate( { 'a': 1 }, { '$set': { 'b': 1 } }, { "returnNewDocument": true } );
   assert.eq( rc, { "_id": 1, "a": 1, "b": 1 } );
   // update，{"returnNewDocument": false}
   var rc = cl.findOneAndUpdate( { 'a': 2 }, { '$set': { 'b': 2 } }, { "returnNewDocument": false } );
   assert.eq( rc, { '_id': 2, 'a': 2 } );

   // update，{"returnNewDocument": true}
   var rc = cl.findOneAndUpdate( { 'a': 3 }, { '$set': { '_id': 3, 'a': 3 } }, { "upsert": true, "returnNewDocument": true } );
   assert.eq( rc, { '_id': 3, 'a': 3 } );
   // upsert，{"returnNewDocument": false}
   var rc = cl.findOneAndUpdate( { 'a': 4 }, { '$set': { '_id': 4, 'a': 4 } }, { "upsert": true, "returnNewDocument": false } );
   assert.eq( rc, null );

   // 检查结果
   var rc = cl.find().sort( { "_id": 1 } );
   checkResults( rc, "[{\"_id\":1,\"a\":1,\"b\":1},{\"_id\":2,\"a\":2,\"b\":2},{\"_id\":3,\"a\":3},{\"_id\":4,\"a\":4}]" );
}

function testFindOneAndUpdateWithSort ( cl )
{
   // returnNewDocument，True更新或插入后的文档，False返回原始文档
   cl.deleteMany( {} );
   cl.insertMany( [{ '_id': 1, 'a': 1 }, { '_id': 2, 'a': 2 }] );

   // 更新多条文档，sort为默认取值
   var rc = cl.findOneAndUpdate( {}, { '$set': { 'b': 1 } }, { "returnNewDocument": true } );
   assert.eq( rc, { "_id": 1, "a": 1, "b": 1 } );
   // 检查结果
   var rc = cl.find().sort( { "_id": 1 } );
   checkResults( rc, "[{\"_id\":1,\"a\":1,\"b\":1},{\"_id\":2,\"a\":2}]" );

   // 更新多条文档，sort:_id字段正序
   var rc = cl.findOneAndUpdate( {}, { '$set': { 'b': 2 } }, { "sort": { "_id": 1 }, "returnNewDocument": true } );
   assert.eq( rc, { "_id": 1, "a": 1, "b": 2 } );
   // 检查结果
   var rc = cl.find().sort( { "_id": 1 } );
   checkResults( rc, "[{\"_id\":1,\"a\":1,\"b\":2},{\"_id\":2,\"a\":2}]" );

   // 更新多条文档，sort:a字段逆序
   var rc = cl.findOneAndUpdate( {}, { '$set': { 'b': 3 } }, { "sort": { "a": -1 }, "returnNewDocument": true } );
   assert.eq( rc, { "_id": 2, "a": 2, "b": 3 } );
   // 检查结果
   var rc = cl.find().sort( { "_id": 1 } );
   checkResults( rc, "[{\"_id\":1,\"a\":1,\"b\":2},{\"_id\":2,\"a\":2,\"b\":3}]" );

   // sort:b字段，b字段无索引
   try
   {
      rc = cl.findOneAndUpdate( {}, { '$set': { 'b': 5 } }, { "sort": { "b": 1 } } );
      assert.eq( "expect fail", "actual success." );
   }
   catch( e )
   {
      assert.eq( e.code, -288 )
   }
   // 检查结果
   var rc = cl.find().sort( { "_id": 1 } );
   checkResults( rc, "[{\"_id\":1,\"a\":1,\"b\":2},{\"_id\":2,\"a\":2,\"b\":3}]" );
}

function testFindOneAndReplace ( cl )
{
   cl.deleteMany( {} );
   cl.insertMany( [{ "_id": 1, "a": 1 }] );

   // 匹配存在的记录，replace字段值
   var rc = cl.findOneAndReplace( { 'a': 1 }, { 'a': 2 } );
   assert.eq( rc, { "_id": 1, "a": 1 } );

   // 匹配存在的记录，replace新的字段和值
   var rc = cl.findOneAndReplace( { 'a': 2 }, { 'b': 1 } );
   assert.eq( rc, { "_id": 1, "a": 2 } );

   // 检查结果
   var rc = cl.find().sort( { "_id": 1 } );
   checkResults( rc, "[{\"_id\":1,\"b\":1}]" );
}

function testFindOneAndReplaceWithUpsert ( cl )
{
   cl.deleteMany( {} );

   // 匹配不存在的记录，{ "upsert": true }
   var rc = cl.findOneAndReplace( { 'a': 1 }, { '_id': 1, 'b': 1 }, { "upsert": true } );
   assert.eq( rc, null );

   // 匹配不存在的记录，{ "upsert": false }
   var rc = cl.findOneAndReplace( { 'a': 2 }, { '_id': 1, 'b': 2 }, { "upsert": false } );
   assert.eq( rc, null );

   // 检查结果
   var rc = cl.find().sort( { "_id": 1 } );
   checkResults( rc, "[{\"_id\":1,\"b\":1}]" );
}

function testFindOneAndReplaceWithReturnNewDocument ( cl )
{
   cl.deleteMany( {} );
   cl.insertMany( [{ '_id': 1, 'a': 1 }, { '_id': 2, 'a': 2 }] );

   // replace，{"returnNewDocument": true}
   var rc = cl.findOneAndReplace( { 'a': 1 }, { 'b': 1 }, { "returnNewDocument": true } );
   assert.eq( rc, { "_id": 1, "b": 1 } );
   // replace，{"returnNewDocument": false}
   var rc = cl.findOneAndReplace( { 'a': 2 }, { 'b': 2 }, { "returnNewDocument": false } );
   assert.eq( rc, { '_id': 2, 'a': 2 } );

   // replace，{"returnNewDocument": true}
   var rc = cl.findOneAndReplace( { 'a': 3 }, { '_id': 3, 'b': 3 }, { "upsert": true, "returnNewDocument": true } );
   assert.eq( rc, { '_id': 3, 'b': 3 } );
   // replace，{"returnNewDocument": false}
   var rc = cl.findOneAndReplace( { 'a': 4 }, { '_id': 4, 'b': 4 }, { "upsert": true, "returnNewDocument": false } );
   assert.eq( rc, null );

   // 检查结果
   var rc = cl.find().sort( { "_id": 1 } );
   checkResults( rc, "[{\"_id\":1,\"b\":1},{\"_id\":2,\"b\":2},{\"_id\":3,\"b\":3},{\"_id\":4,\"b\":4}]" );
}

function testFindOneAndReplaceWithSort ( cl )
{
   // returnNewDocument，True更新或插入后的文档，False返回原始文档
   cl.deleteMany( {} );
   cl.insertMany( [{ '_id': 1, 'a': 1, 'b': 1 }, { '_id': 2, 'a': 2, 'b': 1 }, { '_id': 3, 'a': 3, 'b': 1 }] );

   // 更新文档，sort为默认取值
   var rc = cl.findOneAndReplace( { 'a': 1 }, { 'b': 2 }, { "returnNewDocument": true } );
   assert.eq( rc, { "_id": 1, "b": 2 } );
   // 检查结果
   var rc = cl.find().sort( { "_id": 1 } );
   checkResults( rc, "[{\"_id\":1,\"b\":2},{\"_id\":2,\"a\":2,\"b\":1},{\"_id\":3,\"a\":3,\"b\":1}]" );

   // 更新多条文档，sort:_id字段正序
   var rc = cl.findOneAndReplace( { 'b': 1 }, { 'a': 4, 'b': 2 }, { "sort": { "_id": 1 }, "returnNewDocument": true } );
   assert.eq( rc, { "_id": 2, "a": 4, "b": 2 } );
   // 检查结果
   var rc = cl.find().sort( { "_id": 1 } );
   checkResults( rc, "[{\"_id\":1,\"b\":2},{\"_id\":2,\"a\":4,\"b\":2},{\"_id\":3,\"a\":3,\"b\":1}]" );

   // 更新多条文档，sort:a字段逆序
   var rc = cl.findOneAndReplace( { 'b': 2 }, { 'a': 5 }, { "sort": { "a": -1 }, "returnNewDocument": true } );
   assert.eq( rc, { "_id": 2, "a": 5 } );
   // 检查结果
   var rc = cl.find().sort( { "_id": 1 } );
   checkResults( rc, "[{\"_id\":1,\"b\":2},{\"_id\":2,\"a\":5},{\"_id\":3,\"a\":3,\"b\":1}]" );

   // sort:b字段，b字段无索引
   try
   {
      rc = cl.findOneAndReplace( { 'c': 2 }, { 'b': 4 }, { "sort": { "b": -1 } } );
      assert.eq( "expect fail", "actual success." );
   }
   catch( e )
   {
      assert.eq( e.code, -288 )
   }
   // 检查结果
   var rc = cl.find().sort( { "_id": 1 } );
   checkResults( rc, "[{\"_id\":1,\"b\":2},{\"_id\":2,\"a\":5},{\"_id\":3,\"a\":3,\"b\":1}]" );
}

function testFindOneAndDelete ( cl )
{
   cl.deleteMany( {} );
   cl.insertMany( [{ "_id": 1, "a": 1 }, { "_id": 2, "a": 2 }, { "_id": 3, "a": 3 }] );

   // 匹配多条记录删除
   var rc = cl.findOneAndDelete( { 'a': { '$gte': 2 } } );
   assert.eq( rc, { "_id": 2, "a": 2 } );

   // 检查结果
   var rc = cl.find().sort( { "_id": 1 } );
   checkResults( rc, "[{\"_id\":1,\"a\":1},{\"_id\":3,\"a\":3}]" );
}

function testFindOneAndDeleteWithSort ( cl )
{
   cl.deleteMany( {} );
   cl.insertMany( [{ "_id": 1, "a": 1 }, { "_id": 2, "a": 2 }, { "_id": 3, "a": 3 }] );

   // 匹配多条记录删除，sort:_id字段正序
   var rc = cl.findOneAndDelete( { 'a': { '$gte': 2 } }, { "sort": { "_id": 1 } } );
   assert.eq( rc, { "_id": 2, "a": 2 } );
   // 检查结果
   var rc = cl.find().sort( { "_id": 1 } );
   checkResults( rc, "[{\"_id\":1,\"a\":1},{\"_id\":3,\"a\":3}]" );

   // 匹配多条记录删除，sort:a字段逆序
   var rc = cl.findOneAndDelete( { 'a': { '$gt': 0 } }, { "sort": { "a": -1 } } );
   assert.eq( rc, { "_id": 3, "a": 3 } );
   // 检查结果
   var rc = cl.find().sort( { "_id": 1 } );
   checkResults( rc, "[{\"_id\":1,\"a\":1}]" );

   // sort:b字段，b字段无索引
   try
   {
      rc = cl.findOneAndDelete( { 'a': { '$gt': 0 } }, { "sort": { "b": 1 } } );
      assert.eq( "expect fail", "actual success." );
   }
   catch( e )
   {
      assert.eq( e.code, -288 )
   }
   // 检查结果
   var rc = cl.find().sort( { "_id": 1 } );
   checkResults( rc, "[{\"_id\":1,\"a\":1}]" );
}

function testNotExistDB_findOneAndXXX ( cl )
{
   // db不存在，findOneAndUpdate
   db.dropDatabase();
   var rc = cl.findOneAndUpdate( { 'a': 1 }, { '$set': { '_id': 1 } } )
   assert.eq( rc, null )
   // 检查结果
   var rc = cl.find().sort( { "_id": 1 } );
   checkResults( rc, "[]" );

   // db不存在，findOneAndUpdate, upsert:true
   db.dropDatabase();
   var rc = cl.findOneAndUpdate( { 'a': 2 }, { '$set': { '_id': 2, 'b': 2 } }, { "upsert": true, "returnNewDocument": true } );
   assert.eq( rc, { "_id": 2, "a": 2, "b": 2 } );
   // 检查结果
   var rc = cl.find().sort( { "_id": 1 } );
   checkResults( rc, "[{\"_id\":2,\"a\":2,\"b\":2}]" );

   // db不存在，findOneAndReplace, upsert:true
   db.dropDatabase();
   var rc = cl.findOneAndReplace( { 'a': 3 }, { '_id': 3, 'b': 3 }, { "upsert": true, "returnNewDocument": true } );
   assert.eq( rc, { "_id": 3, "b": 3 } );
   // 检查结果
   var rc = cl.find().sort( { "_id": 1 } );
   checkResults( rc, "[{\"_id\":3,\"b\":3}]" );

   // db不存在，findOneAndDelete
   db.dropDatabase();
   var rc = cl.findOneAndDelete( {} );
   assert.eq( rc, null );
   // 检查结果
   var rc = cl.find().sort( { "_id": 1 } );
   checkResults( rc, "[]" );
}

function testNotExistCL_findOneAndXXX ( cl )
{
   // db不存在，findOneAndUpdate
   cl.drop();
   var rc = cl.findOneAndUpdate( { 'a': 1 }, { '$set': { '_id': 1 } } )
   assert.eq( rc, null )
   // 检查结果
   var rc = cl.find().sort( { "_id": 1 } );
   checkResults( rc, "[]" );

   // db不存在，findOneAndUpdate, upsert:true
   cl.drop();
   var rc = cl.findOneAndUpdate( { 'a': 2 }, { '$set': { '_id': 2, 'b': 2 } }, { "upsert": true, "returnNewDocument": true } );
   assert.eq( rc, { "_id": 2, "a": 2, "b": 2 } );
   // 检查结果
   var rc = cl.find().sort( { "_id": 1 } );
   checkResults( rc, "[{\"_id\":2,\"a\":2,\"b\":2}]" );

   // db不存在，findOneAndReplace, upsert:true
   cl.drop();
   var rc = cl.findOneAndReplace( { 'a': 3 }, { '_id': 3, 'b': 3 }, { "upsert": true, "returnNewDocument": true } );
   assert.eq( rc, { "_id": 3, "b": 3 } );
   // 检查结果
   var rc = cl.find().sort( { "_id": 1 } );
   checkResults( rc, "[{\"_id\":3,\"b\":3}]" );

   // db不存在，findOneAndDelete
   cl.drop();
   var rc = cl.findOneAndDelete( {} );
   assert.eq( rc, null );
   // 检查结果
   var rc = cl.find().sort( { "_id": 1 } );
   checkResults( rc, "[]" );
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