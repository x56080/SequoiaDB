/******************************************************************************
 * @Description   : seqDB-31503:创建全文索引，索引字段为string类型（时间格式）
 * @Author        : wu yan 
 * @CreateTime    : 2023.05.11
 * @LastEditTime  : 2023.05.11
 * @LastEditors   : wu yan
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_es_31503";
testConf.clOpt = { ShardingKey: { date1: 1 }, ShardingType: "hash", AutoSplit: true };


main( test );
function test ( testPara )
{
   var dbcl = testPara.testCL;
   var clName = testConf.clName;
   // 创建全文索引前插入数据
   var doc = [{ no: 1, date1: "2020-01-03", date2: "2023-03-31T12:34:20", date3: "2024-04-23T12:33:59.123", date4: "2022-02-01T02:23:44.333Z" },
   { no: 2, date1: "2000-11-03", date2: "2020-01-03T02:00:00", date3: "2024-03-23T04:34:00.333", date4: "2002-02-01T01:24:10.111Z" },
   { no: 3, date1: "1997-01-03", date2: "2025-03-31T13:34:59", date3: "2054-04-23T08:20:33.111", date4: "1900-12-01T02:35:11.234Z" },
   { no: 4, date1: "2020-11-13", date2: "1901-03-31T22:01:01", date3: "2028-07-01T19:50:00.222", date4: "2012-08-01T19:59:59.999Z" }];
   dbcl.insert( doc );

   var newdoc = [{ no: 5, date1: "2021-01-03", date2: "2015-03-31T13:34:59", date3: "2024-04-23T23:00:45.666", date4: "2022-02-01T04:45:55.222Z" }];

   var indexName1 = "textIndex1_31503";
   var matchconf1 = { "date1": "2020-01-03" };
   var matchconf2 = { "date1": "2021-01-03" };
   testcase( dbcl, clName, indexName1, "date1", matchconf1, matchconf2, doc[0], newdoc );

   var indexName2 = "textIndex2_31503";
   matchconf1 = { "date2": "2025-03-31T13:34:59" };
   matchconf2 = { "date2": "2015-03-31T13:34:59" };
   testcase( dbcl, clName, indexName2, "date2", matchconf1, matchconf2, doc[2], newdoc );

   var indexName3 = "textIndex3_31503";
   matchconf1 = { "date3": "2028-07-01T19:50:00.222" };
   matchconf2 = { "date3": "2024-04-23T23:00:45.666" };
   testcase( dbcl, clName, indexName3, "date3", matchconf1, matchconf2, doc[3], newdoc );

   var indexName4 = "textIndex4_31503";
   matchconf1 = { "date4": "2002-02-01T01:24:10.111Z" };
   matchconf2 = { "date4": "2022-02-01T04:45:55.222Z" };
   testcase( dbcl, clName, indexName4, "date4", matchconf1, matchconf2, doc[1], newdoc );
}

function testcase ( dbcl, clName, indexName, indexField, matchConf1, matchConf2, expectRecords, newInsertRecords )
{
   // 创建索引
   println( "---test index " + indexName );
   var indexObj = {};
   indexObj[indexField] = "text";
   dbcl.createIndex( indexName, indexObj );

   // 全文检索，检查结果 
   var esIndexNames = dbOpr.getESIndexNames( COMMCSNAME, clName, indexName );
   var indexRecordNum = 4;
   checkFullSyncToES( COMMCSNAME, clName, indexName, indexRecordNum, esIndexNames );

   var actRecords = dbOpr.findFromCL( dbcl, { "": { "$Text": { query: { match: matchConf1 } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expectRecords = [expectRecords];
   checkResult( expectRecords, actRecords );

   // 创建全文索引后再次插入同批数据
   dbcl.insert( newInsertRecords );

   // 全文检索，检查结果
   indexRecordNum = 5;
   checkFullSyncToES( COMMCSNAME, clName, indexName, indexRecordNum, esIndexNames );
   actRecords = dbOpr.findFromCL( dbcl, { "": { "$Text": { query: { match: matchConf2 } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   checkResult( newInsertRecords, actRecords );

   // 删除索引
   dbcl.dropIndex( indexName );
   checkIndexNotExistInES( esIndexNames );

   //删除新插入记录
   dbcl.remove( newInsertRecords[0] );
}


