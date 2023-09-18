/********************************************************
 * @Author        : huangxiaoni 
 * @CreateTime    : 2023-09-01
 * @LastEditors   : huangxiaoni 
 * @LastEditTime  : 2023-09-01
 * @Description   : seqDB-33154:删除索引（单个/批量/key方式/runCommand方式）
*********************************************************/

main();
function main ()
{
   var clName = "cl_33154";
   var cl = db.getCollection( clName );
   cl.drop();

   // 准备数据
   cl.insert( { "_id": 1, "a": 1, "b": 1, "c": 1, "d": 1, "e": 1 } );
   cl.createIndex( { "a": 1 }, { "name": "idx_a" } );

   // cl.dropIndex
   // 创建索引
   cl.createIndex( { "b": 1 }, { "name": "idx_b" } );
   cl.createIndex( { "c": -1 }, { "name": "idx_c" } );
   // 指定 name 删索引
   var rc = cl.dropIndex( "idx_b" );
   assert.eq( JSON.stringify( rc ), "{\"ok\":1}" );
   // 指定 key 删索引
   var rc = cl.dropIndex( { "c": -1 } );
   assert.eq( JSON.stringify( rc ), "{\"ok\":1}" );
   // 检查结果
   var rc = cl.getIndexes();
   assert.eq( JSON.stringify( rc ), "[{\"v\":0,\"key\":{\"_id\":1},\"name\":\"_id_\",\"ns\":\"" + cl.toString() + "\"},{\"v\":0,\"key\":{\"a\":1},\"name\":\"idx_a\",\"ns\":\"" + cl.toString() + "\"}]" );

   // 指定 name 删索引，name 不存在
   var rc = cl.dropIndex( "notExist" );
   assert.eq( rc.code, 27 ); // mongo 错误码
   // 指定 key 删索引，key 不存在
   var rc = cl.dropIndex( { "notExist": -1 } );
   assert.eq( rc.code, 27 );
   // 指定 name 删除 _id 索引
   var rc = cl.dropIndex( "$id" );
   assert.eq( rc.code, -32 );
   // 指定 key 删除 _id 索引
   var rc = cl.dropIndex( { "_id": 1 } );
   assert.eq( rc.code, -32 );


   // cl.dropIndexes（mongo 4.4 版本开始支持）
   // 创建索引
   cl.createIndex( { "b": 1 }, { "name": "idx_b" } );
   cl.createIndex( { "c": -1 }, { "name": "idx_c" } );
   // 指定多个 name 批量删索引
   var rc = cl.dropIndexes( ["idx_b", "idx_c"] );
   assert.eq( JSON.stringify( rc ), "{\"ok\":1}" );
   // 检查结果
   var rc = cl.getIndexes();
   assert.eq( JSON.stringify( rc ), "[{\"v\":0,\"key\":{\"_id\":1},\"name\":\"_id_\",\"ns\":\"" + cl.toString() + "\"},{\"v\":0,\"key\":{\"a\":1},\"name\":\"idx_a\",\"ns\":\"" + cl.toString() + "\"}]" );

   // 删除所有索引
   cl.createIndex( { "b": 1 }, { "name": "idx_b" } );
   cl.createIndex( { "c": -1 }, { "name": "idx_c" } );
   var rc = cl.dropIndexes();
   assert.eq( JSON.stringify( rc ), "{\"ok\":1}" );
   // 检查结果
   var rc = cl.getIndexes();
   assert.eq( JSON.stringify( rc ), "[{\"v\":0,\"key\":{\"_id\":1},\"name\":\"_id_\",\"ns\":\"" + cl.toString() + "\"}]" );

   // 指定多个 key 批量删索引
   try
   {
      cl.dropIndexes( [{ "e": 1 }, { "f": 1 }] );
      assert.eq( "actual success", "expect fail" );
   } catch( e )
   {
      assert.eq( e.code, -6 );
   }


   // db.runCommand( { "dropIndex" : <collection>, index: <index> } )
   // 创建索引
   cl.createIndex( { "a": 1 }, { "name": "idx_a" } );
   // 指定 name 删索引，mongo shell 中不支持
   var rc = db.runCommand( { "dropIndex": clName, "index": "idx_a" } );
   assert.eq( rc.code, -32 );
   // 检查结果
   var rc = cl.getIndexes();
   assert.eq( JSON.stringify( rc ), "[{\"v\":0,\"key\":{\"_id\":1},\"name\":\"_id_\",\"ns\":\"" + cl.toString() + "\"},{\"v\":0,\"key\":{\"a\":1},\"name\":\"idx_a\",\"ns\":\"" + cl.toString() + "\"}]" );

   // db.runCommand( { dropIndexes: <collection>, index: <index> } )
   // 创建索引
   cl.createIndex( { "b": 1 }, { "name": "idx_b" } );
   cl.createIndex( { "c": 1 }, { "name": "idx_c" } );
   // 指定多个 name 批量删索引
   var rc = db.runCommand( { "dropIndexes": clName, "index": ["idx_b", "idx_c"] } );
   assert.eq( JSON.stringify( rc ), "{\"ok\":1}" );
   // 指定 key 删索引
   var rc = db.runCommand( { "dropIndexes": clName, "index": [{ "c": 1 }] } );
   assert.eq( rc.code, -6 );
   // 检查结果
   var rc = cl.getIndexes();
   assert.eq( JSON.stringify( rc ), "[{\"v\":0,\"key\":{\"_id\":1},\"name\":\"_id_\",\"ns\":\"" + cl.toString() + "\"},{\"v\":0,\"key\":{\"a\":1},\"name\":\"idx_a\",\"ns\":\"" + cl.toString() + "\"}]" );

   // 指定其他参数：writeConcern，不需要关系 writeConcern 功能，执行不报错即可
   var rc = db.runCommand( { "dropIndexes": clName, "index": ["idx_a"], "writeConcern": { "w": 1 } } );
   assert.eq( JSON.stringify( rc ), "{\"ok\":1}" );
   // 检查结果
   var rc = cl.getIndexes();
   assert.eq( JSON.stringify( rc ), "[{\"v\":0,\"key\":{\"_id\":1},\"name\":\"_id_\",\"ns\":\"" + cl.toString() + "\"}]" );


   // 批量删除索引，包含 _id 索引
   cl.createIndex( { "a": 1 }, { "name": "idx_a" } );
   var rc = db.runCommand( { "dropIndexes": clName, "index": ["idx_a", "_id_"] } );
   assert.eq( rc, { "ok": 0, "code": -32, "errmsg": "cannot drop _id index" } );
   // 检查结果
   var rc = cl.getIndexes();
   assert.eq( JSON.stringify( rc ), "[{\"v\":0,\"key\":{\"_id\":1},\"name\":\"_id_\",\"ns\":\"" + cl.toString() + "\"},{\"v\":0,\"key\":{\"a\":1},\"name\":\"idx_a\",\"ns\":\"" + cl.toString() + "\"}]" );
   cl.dropIndexes();


   // 批量删除索引，部分索引不存在
   cl.createIndex( { "a": 1 }, { "name": "idx_a" } );
   cl.createIndex( { "b": 1 }, { "name": "idx_b" } );
   // 非最后1个索引不存在
   var rc = db.runCommand( { "dropIndexes": clName, "index": ["idx_a1", "idx_a2", "idx_a"] } );
   assert.eq( rc, { "ok": 1 } );
   // 最后一个索引不存在
   var rc = db.runCommand( { "dropIndexes": clName, "index": ["idx_b", "idx_a1"] } );
   assert.eq( rc.ok, 1 );
   assert.eq( rc.code, 27 );
   // 检查结果
   var rc = cl.getIndexes();
   assert.eq( JSON.stringify( rc ), "[{\"v\":0,\"key\":{\"_id\":1},\"name\":\"_id_\",\"ns\":\"" + cl.toString() + "\"}]" );


   // 检查表数据
   var rc = cl.find();
   checkResults( rc, "[{\"_id\":1,\"a\":1,\"b\":1,\"c\":1,\"d\":1,\"e\":1}]" );


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