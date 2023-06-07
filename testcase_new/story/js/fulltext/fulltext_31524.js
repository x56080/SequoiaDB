/******************************************************************************
 * @Description   : seqDB-31524:创建全文索引，更新索引字段值（兼容映射类型）
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.05.29
 * @LastEditTime  : 2023.05.29
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_es_31524";

main( test );
function test ( testPara )
{
   var dbcl = testPara.testCL;
   var clName = testConf.clName;

   // 创建全文索引
   var idxName = "idx_31524";
   dbcl.createIndex( idxName, { "tstr": "text" } );

   // 插入数据
   var docs = [
      { tstr: "123", tdate: { "$date": "1970-01-01" }, tint: 2147483647, tobj: { "city": "guangzhou" } },
      { tstr: "abc", tdate: { "$date": "2020-12-12" }, tint: 0, tobj: { a: 1.001 } },
      { tstr: "字符串", tdate: { "$date": "2023-05-25" }, tint: -2147483648, tobj: { a: 1, b: 2 } }
   ];
   dbcl.insert( docs );

   // 检查全文索引结果
   var dbOpr = new DBOperator();
   var esIndexNames = dbOpr.getESIndexNames( COMMCSNAME, clName, idxName );
   var indexRecordNum = 3;
   checkFullSyncToES( COMMCSNAME, clName, idxName, indexRecordNum, esIndexNames );
   var actRecords1 = dbOpr.findFromCL( dbcl, { "": { "$Text": { query: { "match_all": {} } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expRecords1 = dbOpr.findFromCL( dbcl, {}, { _id: { "$include": 0 } }, { _id: 1 } );
   checkResult( expRecords1, actRecords1 );

   // 更新索引字段为相同类型
   dbcl.update( { "$set": { "tstr": "456" } }, { "tstr": "123" } );
   checkFullSyncToES( COMMCSNAME, clName, idxName, indexRecordNum, esIndexNames );
   var actRecords2 = dbOpr.findFromCL( dbcl, { "": { "$Text": { query: { "match": { "tstr": "456" } } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expRecords2 = dbOpr.findFromCL( dbcl, { "tstr": "456" }, { _id: { "$include": 0 } }, { _id: 1 } );
   checkResult( expRecords2, actRecords2 );

   // 更新索引字段为不同类型(可兼容类型)
   // int
   dbcl.update( { "$set": { "tstr": 2147483647 } }, { "tstr": "456" } );
   checkFullSyncToES( COMMCSNAME, clName, idxName, indexRecordNum, esIndexNames );
   var actRecords3 = dbOpr.findFromCL( dbcl, { "": { "$Text": { query: { "match": { "tstr": "2147483647" } } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expRecords3 = dbOpr.findFromCL( dbcl, { "tstr": 2147483647 }, { _id: { "$include": 0 } }, { _id: 1 } );
   checkResult( expRecords3, actRecords3 );

   // long
   dbcl.update( { "$set": { "tstr": { "$numberLong": "2147483647" } } }, { "tstr": "2147483647" } );
   checkFullSyncToES( COMMCSNAME, clName, idxName, indexRecordNum, esIndexNames );
   var actRecords4 = dbOpr.findFromCL( dbcl, { "": { "$Text": { query: { "match": { "tstr": "2147483647" } } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expRecords4 = dbOpr.findFromCL( dbcl, { "tstr": { "$numberLong": "2147483647" } }, { _id: { "$include": 0 } }, { _id: 1 } );
   checkResult( expRecords4, actRecords4 );

   // double
   dbcl.update( { "$set": { "tstr": -1.7e+308 } }, { "tstr": { "$numberLong": "2147483647" } } );
   checkFullSyncToES( COMMCSNAME, clName, idxName, indexRecordNum, esIndexNames );
   var actRecords5 = dbOpr.findFromCL( dbcl, { "": { "$Text": { query: { "match": { "tstr": "-1.7e+308" } } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expRecords5 = dbOpr.findFromCL( dbcl, { "tstr": -1.7e+308 }, { _id: { "$include": 0 } }, { _id: 1 } );
   checkResult( expRecords5, actRecords5 );

   // float 
   dbcl.update( { "$set": { "tstr": 3.4e+38 } }, { "tstr": "-1.7e+308" } );
   checkFullSyncToES( COMMCSNAME, clName, idxName, indexRecordNum, esIndexNames );
   var actRecords6 = dbOpr.findFromCL( dbcl, { "": { "$Text": { query: { "match": { "tstr": "3.4e+38" } } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expRecords6 = dbOpr.findFromCL( dbcl, { "tstr": 3.4e+38 }, { _id: { "$include": 0 } }, { _id: 1 } );
   checkResult( expRecords6, actRecords6 );

   // date
   dbcl.update( { "$set": { "tstr": { "$date": "1970-01-01" } } }, { "tstr": "3.4e+38" } );
   checkFullSyncToES( COMMCSNAME, clName, idxName, indexRecordNum, esIndexNames );
   var actRecords7 = dbOpr.findFromCL( dbcl, { "": { "$Text": { query: { "match": { "tstr": "1970-01-01" } } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expRecords7 = dbOpr.findFromCL( dbcl, { "tstr": { "$date": "1970-01-01" } }, { _id: { "$include": 0 } }, { _id: 1 } );
   checkResult( expRecords7, actRecords7 );

   // bool
   dbcl.update( { "$set": { "tstr": true } }, { "tstr": { "$date": "1970-01-01" } } );
   checkFullSyncToES( COMMCSNAME, clName, idxName, indexRecordNum, esIndexNames );
   var actRecords8 = dbOpr.findFromCL( dbcl, { "": { "$Text": { query: { "match": { "tstr": true } } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expRecords8 = dbOpr.findFromCL( dbcl, { "tstr": true }, { _id: { "$include": 0 } }, { _id: 1 } );
   checkResult( expRecords8, actRecords8 );

   // timestamp
   dbcl.update( { "$set": { "tstr": { "$timestamp": "2012-01-01-13.14.26.124233" } } }, { "tstr": true } );
   checkFullSyncToES( COMMCSNAME, clName, idxName, indexRecordNum, esIndexNames );
   var actRecords9 = dbOpr.findFromCL( dbcl, { "": { "$Text": { query: { "match": { "tstr": "2012-01-01-13.14.26.124233" } } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expRecords9 = dbOpr.findFromCL( dbcl, { "tstr": { "$timestamp": "2012-01-01-13.14.26.124233" } }, { _id: { "$include": 0 } }, { _id: 1 } );

   // array
   dbcl.update( { "$set": { "tstr": ["arr1", "arr2"] } }, { "tstr": { "$timestamp": "2012-01-01-13.14.26.124233" } } );
   checkFullSyncToES( COMMCSNAME, clName, idxName, indexRecordNum, esIndexNames );
   var actRecords10 = dbOpr.findFromCL( dbcl, { "": { "$Text": { query: { "match": { "tstr": "arr1" } } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expRecords10 = dbOpr.findFromCL( dbcl, { "tstr": ["arr1", "arr2"] }, { _id: { "$include": 0 } }, { _id: 1 } );
   checkResult( expRecords10, actRecords10 );

   // 删除索引
   dbcl.dropIndex( idxName );
   checkIndexNotExistInES( esIndexNames );
}