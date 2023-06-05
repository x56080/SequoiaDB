/******************************************************************************
 * @Description   : seqDB-31514 :: 创建全文索引，索引字段值为多个满足映射类型
 * @Author        : wu yan 
 * @CreateTime    : 2023.05.19
 * @LastEditTime  : 2023.05.19
 * @LastEditors   : wu yan
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_es_31514";

main( test );
function test ( testPara )
{
   var dbcl = testPara.testCL;
   var clName = testConf.clName;

   var indexName = "index_31514";
   dbcl.createIndex( indexName, { test: "text" } );
   var doc = [{ no: 1, test: "guangzhou", testb: true },
   { no: 2, test: { "city": "shanghai" }, testb: null },
   { no: 3, test: 32, testb: { "test": 1243 } },
   { no: 4, test: { "$numberLong": "3000000000" }, testb: { "$numberLong": "1234500000" } },
   { no: 5, test: -1.7e+308, testb: "false" },
   { no: 6, test: true, testb: 234.567 },
   { no: 7, test: { "$date": "2024-02-01" }, testb: { "$date": "2023-02-01" } },
   { no: 8, test: { "$timestamp": "2022-01-01-13.14.26.124233" }, testb: { "$timestamp": "2023-04-11-13.14.26.124233" } },
   { no: 9, test: { "test": 1002 }, testb: { "test": "true" } },
   { no: 10, test: ["arr01", "arr02"], testb: ["true", "false", "false"] }];
   dbcl.insert( doc );

   //全文检索数据，检查结果   
   var indexRecordNum = 8;
   checkFullSyncToES( COMMCSNAME, clName, indexName, indexRecordNum );
   var actRecords1 = dbOpr.findFromCL( dbcl, { "": { "$Text": { "query": { "match_all": {} } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expectRecords1 = [{ no: 1, test: "guangzhou", testb: true },
   { no: 3, test: 32, testb: { "test": 1243 } },
   { no: 4, test: 3000000000, testb: 1234500000 },
   { no: 5, test: -1.7e+308, testb: "false" },
   { no: 6, test: true, testb: 234.567 },
   { no: 7, test: { "$date": "2024-02-01" }, testb: { "$date": "2023-02-01" } },
   { no: 8, test: { "$timestamp": "2022-01-01-13.14.26.124233" }, testb: { "$timestamp": "2023-04-11-13.14.26.124233" } },
   { no: 10, test: ["arr01", "arr02"], testb: ["true", "false", "false"] }];
   checkResult( expectRecords1, actRecords1 );

   // 重建索引
   dbcl.dropIndex( indexName );
   dbcl.createIndex( indexName, { "testb": "text" } );
   var expIndexRecordNum = 3;
   checkFullSyncToES( COMMCSNAME, clName, indexName, expIndexRecordNum );
   var actRecords2 = dbOpr.findFromCL( dbcl, { "": { "$Text": { "query": { "match_all": {} } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expectRecords2 = [{ no: 1, no: 1, test: "guangzhou", testb: true },
   { no: 5, test: -1.7e+308, testb: "false" },
   { no: 10, test: ["arr01", "arr02"], testb: ["true", "false", "false"] }];
   checkResult( expectRecords2, actRecords2 );
}


