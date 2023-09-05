/******************************************************************************
 * @Description   : seqDB-33153:listDatabases
 * @Author        : XiaoNi Huang
 * @CreateTime    : 2023.08.31
 * @LastEditTime  : 2023.08.31
 * @LastEditors   : XiaoNi Huang
 ******************************************************************************/

main();
function main ()
{
   var clName = "cl_33153";
   var rc;
   db.dropDatabase();
   db.getCollection( clName ).insert( { "name": "test" } );

   // 不带参数
   var rc = db.runCommand( { "listDatabases": 1 } );
   assert.neq( rc.databases.length, 0 );

   // 带参数: filter
   // filter: {}
   var rc = db.runCommand( { "listDatabases": 1, "filter": {} } );
   assert.neq( rc.databases.length, 0 );
   // filter: { "name": "xxx" }
   var rc = db.runCommand( { "listDatabases": 1, "filter": { "name": db.getName() } } );
   assert.eq( rc.databases[0].name, db.getName() );
   // filter 带匹配符，不支持，同原生 mongo 引擎
   var rc = db.runCommand( { "listDatabases": 1, "filter": { "name": { "$eq": db.getName() } } } );
   assert.eq( rc.code, -6 );

   // 带参数: nameOnly
   // nameOnly: true
   var rc = db.runCommand( { "listDatabases": 1, "filter": { "name": db.getName() }, "nameOnly": true } );
   assert.eq( rc.databases[0].name, db.getName() );
   // nameOnly: false
   var rc = db.runCommand( { "listDatabases": 1, "filter": { "name": db.getName() }, "nameOnly": false } );
   assert.eq( rc.databases[0].name, db.getName() );
   // 带参数: authorizedCollections
   // authorizedCollections: true
   var rc = db.runCommand( { "listDatabases": 1, "filter": { "name": db.getName() }, "authorizedCollections": true } );
   assert.eq( rc.databases[0].name, db.getName() );
   // authorizedCollections: false
   var rc = db.runCommand( { "listDatabases": 1, "filter": { "name": db.getName() }, "authorizedCollections": false } );
   assert.eq( rc.databases[0].name, db.getName() );
}