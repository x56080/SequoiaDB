/******************************************************************************
 * @Description   : seqDB-32904:find/createIndexes操作时使用maxTimeMS参数
 * @Author        : XiaoNi Huang
 * @CreateTime    : 2023.08.16
 * @LastEditTime  : 2023.08.16
 * @LastEditors   : XiaoNi Huang
 ******************************************************************************/

main();
function main ()
{
   var clName = "cl_32904";
   var cl = db.getCollection( clName );
   cl.drop();

   var docsNum = 20000;
   var docs = [];
   for( var i = 0; i < docsNum; i++ )
   {
      docs.push( { "a": i, "b": i } );
   }
   cl.insert( docs );

   // maxTimeMS 大于实际执行时间
   var maxTimeMS = 10 * 1000;
   // find
   var rc = cl.find( { a: { "$gte": 100 } } ).sort( { "a": -1 } ).maxTimeMS( maxTimeMS );
   assert.eq( rc.size(), docsNum - 100 );

   // createIndexes
   var rc = db.runCommand( { "createIndexes": clName, "indexes": [{ "key": { "a": -1 }, "name": "aIdx" }, { "key": { "b": -1 }, "name": "bIdx" }], "maxTimeMS": maxTimeMS } );
   assert.eq( rc, { "ok": 1 } );
   var rc = cl.getIndexes();
   assert.eq( JSON.stringify( rc.sort() ), "[{\"v\":0,\"key\":{\"_id\":1},\"name\":\"_id_\",\"ns\":\"" + cl.toString() + "\"},{\"v\":0,\"key\":{\"a\":-1},\"name\":\"aIdx\",\"ns\":\"" + cl.toString() + "\"},{\"v\":0,\"key\":{\"b\":-1},\"name\":\"bIdx\",\"ns\":\"" + cl.toString() + "\"}]" );
   cl.dropIndex( "aIdx" );
   cl.dropIndex( "bIdx" );

   // runCommand, distinct
   var rc = db.runCommand( { "distinct": clName, "key": "a", "maxTimeMS": maxTimeMS } );
   assert.eq( rc.values.length, docsNum );


   // maxTimeMS < 实际执行时间
   var maxTimeMS = 10;
   // find
   try
   {
      cl.find( { a: { "$gte": 100 } } ).sort( { "a": -1 } ).maxTimeMS( maxTimeMS );
   }
   catch( e )
   {
      assert.eq( e, 'Error: command failed: { "ok" : 10, "errmsg" : "errmsg": "Operation exceeded time limit", "code" : -116 } : count failed' );
   }

   // createIndexes
   var rc = db.runCommand( { "createIndexes": clName, "indexes": [{ "key": { "a": -1 }, "name": "aIdx" }, { "key": { "b": -1 }, "name": "bIdx" }], "maxTimeMS": maxTimeMS } );
   assert.eq( rc, { "ok": 0, "code": -116, "codeName": "Application is interrupted", "errmsg": "Operation exceeded time limit" } );
   var rc = cl.getIndexes();
   assert.eq( JSON.stringify( rc.sort() ), "[{\"v\":0,\"key\":{\"_id\":1},\"name\":\"_id_\",\"ns\":\"" + cl.toString() + "\"}]" );

   // runCommand, distinct
   var rc = db.runCommand( { "distinct": clName, "key": "a", "maxTimeMS": maxTimeMS } );
   assert.eq( rc, { "ok": 0, "code": -116, "codeName": "Application is interrupted", "errmsg": "Operation exceeded time limit" } );

   cl.drop();
}