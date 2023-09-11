/******************************************************************************
 * @Description   : seqDB-32901:CRUD/aggregate操作，匹配条件为{a:null}
 * @Author        : XiaoNi Huang
 * @CreateTime    : 2023.08.16
 * @LastEditTime  : 2023.08.16
 * @LastEditors   : XiaoNi Huang
 ******************************************************************************/
load( "../fap3-js-mongo4.0/common.js" );

main();
function main ()
{
   var clName = "cl_32901";
   var cl = db.getCollection( clName );
   cl.drop();
   var nullDocs = [{ "_id": 1 }, { "_id": 2, "a": null }];
   var notnullDocs = [{ "_id": 3, "a": 2 }];
   cl.insert( nullDocs );
   cl.insert( notnullDocs );

   // findOne
   var rc = cl.findOne( { "a": null } );
   assert.eq( rc, nullDocs[0] );

   // find
   var rc = cl.find( { "a": null } );
   checkResults( rc, JSON.stringify( nullDocs ) );

   // find with query operators
   var rc = cl.find( { "a": { "$eq": null } } );
   checkResults( rc, JSON.stringify( nullDocs ) );

   var rc = cl.find( { "a": { "$ne": null } } );
   checkResults( rc, JSON.stringify( notnullDocs ) );

   var rc = cl.find( { "a": { "$isnull": 1 } } );
   checkResults( rc, JSON.stringify( nullDocs ) );

   // aggregate
   var rc = cl.aggregate( { "$match": { "a": null } } );
   // SEQUOIADBMAINSTREAM-9845，已修复
   checkResults( rc, JSON.stringify( nullDocs ) );

   var rc = cl.aggregate( { "$match": { "a": { "$isnull": 1 } } } );
   checkResults( rc, JSON.stringify( nullDocs ) );

   cl.drop();
}