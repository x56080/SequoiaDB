/******************************************************************************
 * @Description   : seqDB-24121:findAndModify操作
 * @Author        : XiaoNi Huang
 * @CreateTime    : 2021.04.22
 * @LastEditTime  : 2021.05.06
 * @LastEditors   : XiaoNi Huang
 ******************************************************************************/
main();

function main ()
{
   var clName = "cl24121";
   var cl = db.getCollection( clName );
   cl.drop();

   // ready data
   var docs = [{ "_id": 1, "a": 1, "b": "insert" },
   { "_id": 2, "a": 2, "b": "insert" },
   { "_id": 3, "a": 3, "b": "insert" },
   { "_id": 4, "a": 4, "b": "insert" },
   { "_id": 5, "a": 5, "b": "insert" }];

   // SEQUOIADBMAINSTREAM-7115
   // testFindAndModify( cl );
   testFindAndModifyWithUpdate( cl );
   testFindAndModifyWithRemove( cl );

   cl.drop();
}

function testFindAndModify ( cl )
{
   // 参数格式错误
   try
   {
      cl.findAndModify( {}, { "$set": { "$set": { "b": 1 } } } );
      throw new Error( "expect fail but actual success." );
   }
   catch( e )
   {
      assert.eq( e.code, -32 );
   }

   // remove:false
   try
   {
      cl.findAndModify( { "query": {}, "remove": false } );
      throw new Error( "expect fail but actual success." );
   }
   catch( e )
   {
      assert.eq( e.code, -32 );
   }
}

function testFindAndModifyWithUpdate ( cl )
{
   cl.deleteMany( {} );
   cl.insertMany( [{ '_id': 1, 'a': 1 }, { '_id': 2, 'a': 1 }, { '_id': 3, 'a': 1 }] );

   // update
   var rc = cl.findAndModify( { "query": {}, "update": { "$set": { "a": 2 } } } );
   assert.eq( rc, { "_id": 1, "a": 1 } );
   // update and remove:false
   var rc = cl.findAndModify( { "query": { "_id": { "$gt": 1 } }, "update": { "$set": { "a": -1 } }, "remove": false, "fields": {} } );
   assert.eq( rc, { "_id": 2, "a": 1 } );
   // 检查结果
   var rc = cl.find().sort( { "_id": 1 } );
   checkResults( rc, "[{\"_id\":1,\"a\":2},{\"_id\":2,\"a\":-1},{\"_id\":3,\"a\":1}]" );

   // sort正序，new返回更新前的记录
   var rc = cl.findAndModify( { "query": { "_id": { "$gt": 1 } }, "update": { "$set": { "a": 3 } }, "sort": { "_id": 1 }, "new": false, "fields": { "_id": 1 } } );
   assert.eq( rc, { "_id": 2 } );
   // sort逆序，new返回更新后的记录
   var rc = cl.findAndModify( { "query": { "a": { "$gt": 1 } }, "update": { "$set": { "a": 4 } }, "sort": { "_id": 1 }, "new": false, "fields": { "_id": 1, "a": 1, "c": 1 }, "upsert": true } );
   assert.eq( rc, { "_id": 1, "a": 2 } );
   // 检查结果
   var rc = cl.find().sort( { "_id": 1 } );
   checkResults( rc, "[{\"_id\":1,\"a\":4},{\"_id\":2,\"a\":3},{\"_id\":3,\"a\":1}]" );

   // 匹配不存在的字段
   var rc = cl.findAndModify( { "query": { "_id": 4 }, "update": { "$set": { "a": 4 } } } );
   assert.eq( rc, null );
   // findAndModify, 匹配不存在的字段
   // upsert:false
   var rc = cl.findAndModify( { "query": { "_id": 5 }, "update": { "$set": { "a": 5 } }, "upsert": false } );
   assert.eq( rc, null );
   // upsert:true
   var rc = cl.findAndModify( { "query": { "_id": 6 }, "update": { "$set": { "a": 6 } }, "upsert": true } );
   assert.eq( rc, null );
   // 检查结果
   var rc = cl.find().sort( { "_id": 1 } );
   checkResults( rc, "[{\"_id\":1,\"a\":4},{\"_id\":2,\"a\":3},{\"_id\":3,\"a\":1},{\"_id\":6,\"a\":6}]" );

   // findAndModify, update不带更新符
   var rc = cl.findAndModify( { "query": { "_id": 4 }, "update": { "a": 7 } } );
   assert.eq( rc, null );
   // 检查结果
   var rc = cl.find().sort( { "_id": 1 } );
   checkResults( rc, "[{\"_id\":1,\"a\":4},{\"_id\":2,\"a\":3},{\"_id\":3,\"a\":1},{\"_id\":6,\"a\":6}]" );
}

function testFindAndModifyWithRemove ( cl )
{
   cl.deleteMany( {} );

   // remove:true
   cl.insertMany( [{ '_id': 1, 'a': 1 }, { '_id': 2, 'a': 2 }, { '_id': 3, 'a': 3 }, { '_id': 4, 'a': 4 }, { '_id': 5, 'a': 5 }] );
   var rc = cl.findAndModify( { "query": {}, "remove": true } );
   assert.eq( rc, { "_id": 1, "a": 1 } );

   // 指定sort正序，new:false
   var rc = cl.findAndModify( { "query": {}, "remove": true, "sort": { "_id": 1 }, "new": false } );
   assert.eq( rc, { "_id": 2, "a": 2 } );

   // 指定sort逆序，new:true
   var rc = cl.findAndModify( { "query": {}, "remove": true, "sort": { "_id": -1 }, "new": false } );
   assert.eq( rc, { "_id": 5, "a": 5 } );

   // 检查结果
   var rc = cl.find().sort( { "_id": 1 } );
   checkResults( rc, "[{\"_id\":3,\"a\":3},{\"_id\":4,\"a\":4}]" );

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