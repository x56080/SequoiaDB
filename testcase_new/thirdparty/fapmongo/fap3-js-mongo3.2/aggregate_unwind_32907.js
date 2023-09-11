
/******************************************************************************
 * @Description   : seqDB-32907:aggregate使用$unwind操作符
 *    参数：path、preserveNullAndEmptyArrays、includeArrayIndex
 * @Author        : XiaoNi Huang
 * @CreateTime    : 2023.08.16
 * @LastEditTime  : 2023.08.16
 * @LastEditors   : XiaoNi Huang
 ******************************************************************************/
load( "../fap3-js-mongo4.0/common.js" );

main();
function main ()
{
   var clName = "cl_32907";
   var cl = db.getCollection( clName );
   cl.drop();

   // 准备数据，包括：数组（非空数组、空数组、嵌套数组）、非数组（null、其他）、字段不存在
   cl.insert( [
      { "_id": 1, "a": [1, 2], "c": 1 },
      { "_id": 2, "a": [2, 3], "c": 2 },
      { "_id": 3, "a": [3], "c": 3 },
      { "_id": 4, "a": [], "c": 4 },
      { "_id": 5, "a": [{ "a1": [1, 2] }, { "a1": [3, 4] }], "c": 5 },
      { "_id": 6, "a": [null, 6], "c": 6 },
      { "_id": 7, "a": null, "c": 7 },
      { "_id": 8, "a": 8, "c": 8 },
      { "_id": 9, "a": "", "c": 9 },
      { "_id": 10, "c": 10 }
   ] );

   // $unwind 不带参数
   var rc = cl.aggregate( [{ "$unwind": "$a" }] );
   checkResults( rc, "[{\"_id\":1,\"a\":1,\"c\":1},{\"_id\":1,\"a\":2,\"c\":1},{\"_id\":2,\"a\":2,\"c\":2},{\"_id\":2,\"a\":3,\"c\":2},{\"_id\":3,\"a\":3,\"c\":3},{\"_id\":5,\"a\":{\"a1\":[1,2]},\"c\":5},{\"_id\":5,\"a\":{\"a1\":[3,4]},\"c\":5},{\"_id\":6,\"a\":null,\"c\":6},{\"_id\":6,\"a\":6,\"c\":6},{\"_id\":8,\"a\":8,\"c\":8},{\"_id\":9,\"a\":\"\",\"c\":9}]" );

   // $unwind 仅带 path
   var rc = cl.aggregate( [{ "$unwind": { "path": "$a" } }] );
   checkResults( rc, "[{\"_id\":1,\"a\":1,\"c\":1},{\"_id\":1,\"a\":2,\"c\":1},{\"_id\":2,\"a\":2,\"c\":2},{\"_id\":2,\"a\":3,\"c\":2},{\"_id\":3,\"a\":3,\"c\":3},{\"_id\":5,\"a\":{\"a1\":[1,2]},\"c\":5},{\"_id\":5,\"a\":{\"a1\":[3,4]},\"c\":5},{\"_id\":6,\"a\":null,\"c\":6},{\"_id\":6,\"a\":6,\"c\":6},{\"_id\":8,\"a\":8,\"c\":8},{\"_id\":9,\"a\":\"\",\"c\":9}]" );

   // $unwind 带 path、preserveNullAndEmptyArrays
   // preserveNullAndEmptyArrays: false
   var rc = cl.aggregate( [{ "$unwind": { "path": "$a", "preserveNullAndEmptyArrays": false } }] );
   checkResults( rc, "[{\"_id\":1,\"a\":1,\"c\":1},{\"_id\":1,\"a\":2,\"c\":1},{\"_id\":2,\"a\":2,\"c\":2},{\"_id\":2,\"a\":3,\"c\":2},{\"_id\":3,\"a\":3,\"c\":3},{\"_id\":5,\"a\":{\"a1\":[1,2]},\"c\":5},{\"_id\":5,\"a\":{\"a1\":[3,4]},\"c\":5},{\"_id\":6,\"a\":null,\"c\":6},{\"_id\":6,\"a\":6,\"c\":6},{\"_id\":8,\"a\":8,\"c\":8},{\"_id\":9,\"a\":\"\",\"c\":9}]" );
   // preserveNullAndEmptyArrays: true
   // { "_id" : 4, "a" : [ ], "c" : 4 } 原生mongo引擎返回 { "_id" : 4, "c" : 4 }，fapmongo返回 { "_id" : 4, "a" : null, "c" : 4 }，fapmongo返回结果合理
   var rc = cl.aggregate( [{ "$unwind": { "path": "$a", "preserveNullAndEmptyArrays": true } }] );
   checkResults( rc, "[{\"_id\":1,\"a\":1,\"c\":1},{\"_id\":1,\"a\":2,\"c\":1},{\"_id\":2,\"a\":2,\"c\":2},{\"_id\":2,\"a\":3,\"c\":2},{\"_id\":3,\"a\":3,\"c\":3},{\"_id\":4,\"a\":null,\"c\":4},{\"_id\":5,\"a\":{\"a1\":[1,2]},\"c\":5},{\"_id\":5,\"a\":{\"a1\":[3,4]},\"c\":5},{\"_id\":6,\"a\":null,\"c\":6},{\"_id\":6,\"a\":6,\"c\":6},{\"_id\":7,\"a\":null,\"c\":7},{\"_id\":8,\"a\":8,\"c\":8},{\"_id\":9,\"a\":\"\",\"c\":9},{\"_id\":10,\"c\":10}]" );

   // $unwind 带 $includeArrayIndex，即返回数组元素下标
   // { "_id": 5, "a": [{ "a1": [1, 2] }, { "a1": [3, 4] }], "c": 5 } 原生mongo引擎跟fapmongo拆分后的2条记录返回顺序不同，不影响
   // 3.2版本 arrayIndex 字段返回 {"floatApprox":0}，3.4及以上版本返回 {"$numberLong\":0}
   var rc = cl.aggregate( [{ "$unwind": { "path": "$a", "includeArrayIndex": "arrayIndex" } }] );
   checkResults( rc, "[{\"_id\":1,\"a\":1,\"c\":1,\"arrayIndex\":{\"floatApprox\":0}},{\"_id\":1,\"a\":2,\"c\":1,\"arrayIndex\":{\"floatApprox\":1}},{\"_id\":2,\"a\":2,\"c\":2,\"arrayIndex\":{\"floatApprox\":0}},{\"_id\":2,\"a\":3,\"c\":2,\"arrayIndex\":{\"floatApprox\":1}},{\"_id\":3,\"a\":3,\"c\":3,\"arrayIndex\":{\"floatApprox\":0}},{\"_id\":5,\"a\":{\"a1\":[1,2]},\"c\":5,\"arrayIndex\":{\"floatApprox\":0}},{\"_id\":5,\"a\":{\"a1\":[3,4]},\"c\":5,\"arrayIndex\":{\"floatApprox\":1}},{\"_id\":6,\"a\":null,\"c\":6,\"arrayIndex\":{\"floatApprox\":0}},{\"_id\":6,\"a\":6,\"c\":6,\"arrayIndex\":{\"floatApprox\":1}},{\"_id\":8,\"a\":8,\"c\":8,\"arrayIndex\":null},{\"_id\":9,\"a\":\"\",\"c\":9,\"arrayIndex\":null}]" );

   // $unwind + 其他聚集符
   var rc = cl.aggregate( [{ "$unwind": { "path": "$a", "preserveNullAndEmptyArrays": true } }, { "$group": { "_id": "$a", "averageC": { "$avg": "$c" } } }, { "$sort": { "averageC": -1 } }] );
   checkResults( rc, "[{\"_id\":\"\",\"averageC\":9},{\"_id\":8,\"averageC\":8},{\"_id\":null,\"averageC\":6.75},{\"_id\":6,\"averageC\":6},{\"_id\":{\"a1\":[1,2]},\"averageC\":5},{\"_id\":{\"a1\":[3,4]},\"averageC\":5},{\"_id\":3,\"averageC\":2.5},{\"_id\":2,\"averageC\":1.5},{\"_id\":1,\"averageC\":1}]" );

   cl.drop();
}