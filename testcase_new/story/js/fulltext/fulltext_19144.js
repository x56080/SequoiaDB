/******************************************************************************
 * @Description   : seqDB-19144:单键全文索引，索引字段为数组元素，全量/增量同步
 * @Author        : zhaoyu 
 * @CreateTime    : 2019.08.14
 * @LastEditTime  : 2023.05.25
 * @LastEditors   : wu yan
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_es_19144";

main( test );
function test ( testPara )
{
   var dbcl = testPara.testCL;
   var clName = testConf.clName;
   var textIndexName = "textIndex_19144";

   var objs = new Array( { id: 1, a: "string1" },
      { id: 2, a: 1 },
      { id: 3, a: ["string1", "string2", "string3"] },
      { id: 4, a: [1, 2, 3] },
      { id: 5, a: [{ 0: "obj1" }, { 1: "obj2" }, { 2: "obj3" }] },
      { id: 6, a: [{ 0: 1 }, { 1: 2 }, { 2: 3 }] },
      { id: 7, a: { 1: "obj" } },
      { id: 8, a: [["string4", "string5", "string6"], ["string7", "string8", "string9"], ["string10", "string11", "string12"]] },
      { id: 9, a: [[4, 5, 6], [7, 8, 9], [10, 11, 12]] } );
   dbcl.insert( objs );
   dbcl.createIndex( textIndexName, { "a.1": "text" } );

   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 3 );
   var findCond = { "": { "$Text": { "query": { "match_all": {} } } } };
   var actResult = dbOpr.findFromCL( dbcl, findCond, { "a": { "$include": 1 } }, { _id: 1 } );
   var expResult = [{ a: [{ 0: "obj1" }, { 1: "obj2" }, { 2: "obj3" }] },
   { a: [{ 0: 1 }, { 1: 2 }, { 2: 3 }] },
   { a: { 1: "obj" } }];
   checkResult( expResult, actResult );

   dbcl.insert( objs );
   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 6 );
   var actResult = dbOpr.findFromCL( dbcl, findCond, { "a": { "$include": 1 } }, { _id: 1 } );
   var expResult = [{ a: [{ 0: "obj1" }, { 1: "obj2" }, { 2: "obj3" }] },
   { a: [{ 0: "obj1" }, { 1: "obj2" }, { 2: "obj3" }] },
   { a: [{ 0: 1 }, { 1: 2 }, { 2: 3 }] },
   { a: [{ 0: 1 }, { 1: 2 }, { 2: 3 }] },
   { a: { 1: "obj" } },
   { a: { 1: "obj" } }];

   findCond = { "": { $Text: { query: { match: { "a.1": "obj" } } } } };
   var actResult = dbOpr.findFromCL( dbcl, findCond, { "a": { "$include": 1 } }, { _id: 1 } );
   var expResult = [{ a: { 1: "obj" } },
   { a: { 1: "obj" } }];

   findCond = { "": { $Text: { query: { match: { "a.1": "obj2" } } } } };
   var actResult = dbOpr.findFromCL( dbcl, findCond, { "a": { "$include": 1 } }, { _id: 1 } );
   var expResult = [{ a: [{ 0: "obj1" }, { 1: "obj2" }, { 2: "obj3" }] },
   { a: [{ 0: "obj1" }, { 1: "obj2" }, { 2: "obj3" }] }];

   dbcl.remove();
   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 0 );
}
