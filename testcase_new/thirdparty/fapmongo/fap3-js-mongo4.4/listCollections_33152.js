/******************************************************************************
 * @Description   : seqDB-33152:listCollctions
 * @Author        : XiaoNi Huang
 * @CreateTime    : 2023.08.31
 * @LastEditTime  : 2023.08.31
 * @LastEditors   : XiaoNi Huang
 ******************************************************************************/

main();
function main ()
{
   var rc;
   var clNum = 3;
   db.dropDatabase();

   // 创建多个集合
   var clNameBase = "cl_33152_";
   for( var i = 0; i < clNum; i++ )
   {
      db.createCollection( clNameBase + i );
   }

   // 不带参数
   var rc = db.runCommand( { "listCollections": 1 } );
   assert.eq( JSON.stringify( rc.cursor.firstBatch ), "[{\"name\":\"cl_33152_0\"},{\"name\":\"cl_33152_1\"},{\"name\":\"cl_33152_2\"}]" );

   // 带参数: filter
   // filter: {}
   var rc = db.runCommand( { "listCollections": 1, "filter": {} } );
   assert.eq( JSON.stringify( rc.cursor.firstBatch ), "[{\"name\":\"cl_33152_0\"},{\"name\":\"cl_33152_1\"},{\"name\":\"cl_33152_2\"}]" );
   // filter: { "name": "xxx" }
   var rc = db.runCommand( { "listCollections": 1, "filter": { "name": "cl_33152_1" } } );
   assert.eq( JSON.stringify( rc.cursor.firstBatch ), "[{\"name\":\"cl_33152_1\"}]" );
   // filter 带匹配符，不支持，同原生 mongo 引擎
   var rc = db.runCommand( { "listCollections": 1, "filter": { "name": { "$eq": "cl_33152_1" } } } );
   assert.eq( rc.code, -6 );

   // 带参数: nameOnly
   // nameOnly: true
   var rc = db.runCommand( { "listCollections": 1, "nameOnly": true } );
   assert.eq( JSON.stringify( rc.cursor.firstBatch ), "[{\"name\":\"cl_33152_0\"},{\"name\":\"cl_33152_1\"},{\"name\":\"cl_33152_2\"}]" );
   // nameOnly: false
   var rc = db.runCommand( { "listCollections": 1, "nameOnly": false } );
   assert.eq( JSON.stringify( rc.cursor.firstBatch ), "[{\"name\":\"cl_33152_0\"},{\"name\":\"cl_33152_1\"},{\"name\":\"cl_33152_2\"}]" );

   // 带参数: authorizedCollections
   // authorizedCollections: true
   var rc = db.runCommand( { "listCollections": 1, "authorizedCollections": true } );
   assert.eq( JSON.stringify( rc.cursor.firstBatch ), "[{\"name\":\"cl_33152_0\"},{\"name\":\"cl_33152_1\"},{\"name\":\"cl_33152_2\"}]" );
   // authorizedCollections: false
   var rc = db.runCommand( { "listCollections": 1, "authorizedCollections": false } );
   assert.eq( JSON.stringify( rc.cursor.firstBatch ), "[{\"name\":\"cl_33152_0\"},{\"name\":\"cl_33152_1\"},{\"name\":\"cl_33152_2\"}]" );

   // 清理数据
   for( var i = 0; i < clNum; i++ )
   {
      db.getCollection( clNameBase + i ).drop();
   }
}