/******************************************************************************
 * @Description   : seqDB-33196:insert ordered 基本功能验证
 * @Author        : XiaoNi Huang
 * @CreateTime    : 2023.9.5
 * @LastEditTime  : 2023.9.5
 * @LastEditors   : XiaoNi Huang
 ******************************************************************************/

main();
function main ()
{
   var clName = "cl_33196";
   var cl = db.getCollection( clName );
   cl.drop();
   var rc = cl.insert( [{ "_id": 1, "a": 1 }, { "_id": 2, "a": 2 }, { "_id": 6, "a": 6 }] )

   // ordered: false，记录冲突位置：首条、中间，并存在连续多次冲突记录
   var rc = cl.insert( [{ "_id": 1, "a": 1 }, { "_id": 2, "a": 2 }, { "_id": 3, "a": 3 }, { "_id": 7, "a": 7 }], { "ordered": false } );
   assert.eq( rc, { "writeErrors": [], "writeConcernErrors": [], "nInserted": 2, "nUpserted": 0, "nMatched": 0, "nModified": 0, "nRemoved": 0, "upserted": [] } );
   // 检查数据
   var rc = cl.find().sort( { '_id': 1 } );
   checkResults( rc, "[{\"_id\":1,\"a\":1},{\"_id\":2,\"a\":2},{\"_id\":3,\"a\":3},{\"_id\":6,\"a\":6},{\"_id\":7,\"a\":7}]" );


   // ordered: true，记录冲突时，bulkWrite 会抛异常，insert 会返回错误信息，结果同原生 mongo
   // ordered: true，首条记录冲突
   var rc = cl.insert( [{ "_id": 6, "a": 6 }, { "_id": 8, "a": 8 }], { "ordered": true } );
   assert.eq( rc, {
      "writeErrors": [
         {
            "index": 0,
            "code": -38,
            "errmsg": "Duplicate key exist collection: " + db.getName() + "." + clName + " index: $id dup key: { \"_id\": 6.0 }",
            "op": {
               "_id": 6,
               "a": 6
            }
         }
      ],
      "writeConcernErrors": [],
      "nInserted": 0,
      "nUpserted": 0,
      "nMatched": 0,
      "nModified": 0,
      "nRemoved": 0,
      "upserted": []
   } );
   // 检查数据
   var rc = cl.find().sort( { '_id': 1 } );
   checkResults( rc, "[{\"_id\":1,\"a\":1},{\"_id\":2,\"a\":2},{\"_id\":3,\"a\":3},{\"_id\":6,\"a\":6},{\"_id\":7,\"a\":7}]" );

   // ordered: true，中间记录冲突
   var rc = cl.insert( [{ "_id": 4, "a": 4 }, { "_id": 6, "a": 6 }, { "_id": 8, "a": 8 }], { "ordered": true } );
   assert.eq( rc, {
      "writeErrors": [
         {
            "index": 1,
            "code": -38,
            "errmsg": "Duplicate key exist collection: " + db.getName() + "." + clName + " index: $id dup key: { \"_id\": 6.0 }",
            "op": {
               "_id": 6,
               "a": 6
            }
         }
      ],
      "writeConcernErrors": [],
      "nInserted": 1,
      "nUpserted": 0,
      "nMatched": 0,
      "nModified": 0,
      "nRemoved": 0,
      "upserted": []
   } );
   // 检查数据
   var rc = cl.find().sort( { '_id': 1 } );
   checkResults( rc, "[{\"_id\":1,\"a\":1},{\"_id\":2,\"a\":2},{\"_id\":3,\"a\":3},{\"_id\":4,\"a\":4},{\"_id\":6,\"a\":6},{\"_id\":7,\"a\":7}]" );


   // insertMany（3.2及以上版本支持）
   cl.remove( {} );
   var rc = cl.insertMany( [{ "_id": 1, "a": 1 }, { "_id": 2, "a": 2 }, { "_id": 6, "a": 6 }] )
   // ordered: false，记录冲突位置：首条、中间，并存在连续多次冲突记录
   var rc = cl.insert( [{ "_id": 1, "a": 1 }, { "_id": 2, "a": 2 }, { "_id": 3, "a": 3 }, { "_id": 7, "a": 7 }], { "ordered": false } );
   assert.eq( rc, { "writeErrors": [], "writeConcernErrors": [], "nInserted": 2, "nUpserted": 0, "nMatched": 0, "nModified": 0, "nRemoved": 0, "upserted": [] } );
   // 检查数据
   var rc = cl.find().sort( { '_id': 1 } );
   checkResults( rc, "[{\"_id\":1,\"a\":1},{\"_id\":2,\"a\":2},{\"_id\":3,\"a\":3},{\"_id\":6,\"a\":6},{\"_id\":7,\"a\":7}]" );

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
