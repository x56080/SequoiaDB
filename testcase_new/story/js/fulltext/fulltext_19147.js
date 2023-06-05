/******************************************************************************
 * @Description   : seqDB-19147:单键全文索引，索引字段为数组元素，更新全文索引字段为非string类型
 * @Author        : zhaoyu 
 * @CreateTime    : 2019.08.14
 * @LastEditTime  : 2023.05.12
 * @LastEditors   : wu yan
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_es_19147";
main( test );

function test ()
{
   var dbcl = testPara.testCL;
   var clName = testConf.clName;
   var textIndexName = "textIndex_19147";
   dbcl.createIndex( textIndexName, { "a.1": "text" } );
   var objs = new Array( { id: 1, a: "string1" },
      { id: 2, a: 1 },
      { id: 3, a: ["string1", "string2", "string3"] },
      { id: 4, a: [1, 2, 3] },
      { id: 5, a: [{ 0: "obj1" }, { 1: "obj2" }, { 2: "obj3" }] },
      { id: 6, a: [{ 0: 1 }, { 1: 2 }, { 2: 3 }] },
      { id: 7, a: { 0: "obj", 1: "obj", 2: "obj" } },
      { id: 8, a: [["string4", "string5", "string6"], ["string7", "string8", "string9"], ["string10", "string11", "string12"]] },
      { id: 9, a: [[4, 5, 6], [7, 8, 9], [10, 11, 12]] } );
   dbcl.insert( objs );
   dbcl.update( { $set: { "a.1": 123 } } );

   //更新后只有obj类型满足映射类型，创建索引数据
   var dbOpr = new DBOperator();
   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 1 );
   var findCond = { "": { "$Text": { "query": { "match_all": {} } } } };
   var actResult = dbOpr.findFromCL( dbcl, findCond, { "_id": { "$include": 0 } }, { _id: 1 } );
   var expResult = [{ id: 7, a: { 0: "obj", 1: 123, 2: "obj" } }];
   checkResult( expResult, actResult );
}
