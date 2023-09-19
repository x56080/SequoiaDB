/******************************************************************************
 * @Description   : seqDB-33170:bulkwrite ordered 基本功能验证
 * @Author        : XiaoNi Huang
 * @CreateTime    : 2023.9.5
 * @LastEditTime  : 2023.9.5
 * @LastEditors   : XiaoNi Huang
 ******************************************************************************/

main();
function main ()
{
   var clName = "cl_33170";
   var cl = db.getCollection( clName );
   cl.drop();
   var rc = cl.bulkWrite( [{ "insertOne": { "document": { '_id': 1, 'a': 1 } } }] );

   // ordered: false
   var rc = cl.bulkWrite( [{ "insertOne": { "document": { '_id': 1, 'a': 1 } } }, { "insertOne": { "document": { '_id': 2, 'a': 2 } } }, { "insertOne": { "document": { '_id': 3, 'a': 3 } } }], { "ordered": false } );
   assert.eq( rc, { "acknowledged": true, "deletedCount": 0, "insertedCount": 2, "matchedCount": 0, "upsertedCount": 0, "insertedIds": { "0": 1, "1": 2, "2": 3 }, "upsertedIds": {} } );
   // 检查数据
   var rc = cl.find().sort( { '_id': 1 } );
   checkResults( rc, "[{\"_id\":1,\"a\":1},{\"_id\":2,\"a\":2},{\"_id\":3,\"a\":3}]" );


   // ordered: true
   try
   {
      cl.bulkWrite( [{ "insertOne": { "document": { '_id': 1, 'a': 1 } } }, { "insertOne": { "document": { '_id': 4, 'a': 4 } } }], { "ordered": true } );
      assert.eq( "success", "fail" );
   } catch( e )
   {
      if( JSON.stringify( e ).indexOf( "Duplicate key exist collection" ) == -1 )
      {
         throw e;
      }
   }
   // 检查数据
   var rc = cl.find().sort( { '_id': 1 } );
   checkResults( rc, "[{\"_id\":1,\"a\":1},{\"_id\":2,\"a\":2},{\"_id\":3,\"a\":3}]" );

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
