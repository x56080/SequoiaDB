/******************************************************************************
 * @Description   : seqDB-31515 :: 创建全文索引，索引字段值为包含映射类型和不支持映射类型
 * @Author        : wu yan 
 * @CreateTime    : 2023.05.19
 * @LastEditTime  : 2023.05.19
 * @LastEditors   : wu yan
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_es_31515";

main( test );
function test ( testPara )
{
   var dbcl = testPara.testCL;
   var clName = testConf.clName;

   var indexName = "index_31515";
   dbcl.createIndex( indexName, { test: "text" } );
   var doc = [{ no: 1, test: "guangzhou", testb: null },
   { no: 2, test: { $decimal: "123.456" }, testb: { "city": "shenzhen", "no": 1 } },
   { no: 3, test: { "$oid": "123abcd00ef12358902300ef" }, testb: { "city": "深圳", "no": 1 } },
   { no: 4, test: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" }, testb: { "city": 123.67, "no": { "$numberLong": "1234500000" } } },
   { no: 5, test: { "$regex": "^张", "$options": "i" }, testb: { "city": "true", "no": "0" } },
   { no: 6, test: null, testb: { "city": 45612, "no": 1 } },
   { no: 7, test: { "$minKey": 1 }, testb: { "city": " ", no: [12300000000, 56784444444] } },
   { no: 8, test: { "$maxKey": 1 } }]

   dbcl.insert( doc );

   //全文检索数据，检查结果    
   var indexRecordNum = 1;
   checkFullSyncToES( COMMCSNAME, clName, indexName, indexRecordNum );
   var actRecords1 = dbOpr.findFromCL( dbcl, { "": { "$Text": { "query": { "match_all": {} } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expectRecords1 = [{ no: 1, test: "guangzhou", testb: null }];
   checkResult( expectRecords1, actRecords1 );

   // 重建索引
   dbcl.dropIndex( indexName );
   dbcl.createIndex( indexName, { "testb": "text" } );
   var expIndexRecordNum = 6;
   checkFullSyncToES( COMMCSNAME, clName, indexName, expIndexRecordNum );
   var actRecords2 = dbOpr.findFromCL( dbcl, { "": { "$Text": { "query": { "match_all": {} } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expectRecords2 = [{ no: 2, test: { $decimal: "123.456" }, testb: { "city": "shenzhen", "no": 1 } },
   { no: 3, test: { "$oid": "123abcd00ef12358902300ef" }, testb: { "city": "深圳", "no": 1 } },
   { no: 4, test: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" }, testb: { "city": 123.67, "no": 1234500000 } },
   { no: 5, test: { "$regex": "^张", "$options": "i" }, testb: { "city": "true", "no": "0" } },
   { no: 6, test: null, testb: { "city": 45612, "no": 1 } },
   { no: 7, test: { "$minKey": 1 }, testb: { "city": " ", no: [12300000000, 56784444444] } }];
   checkResult( expectRecords2, actRecords2 );
}


