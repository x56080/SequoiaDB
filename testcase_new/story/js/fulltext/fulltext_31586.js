/******************************************************************************
 * @Description   : seqDB-31586 :: 创建全文索引指定映射为date类型，插入索引字段类型为兼容转换类型
 * @Author        : wu yan 
 * @CreateTime    : 2023.05.19
 * @LastEditTime  : 2023.05.19
 * @LastEditors   : wu yan
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_es_31586";

main( test );
function test ( testPara )
{
   var dbcl = testPara.testCL;
   var clName = testConf.clName;

   var indexName = "index_31586";
   dbcl.createIndex( indexName, { "tdate": "text" }, { "Mappings": { "Fields": { "tdate": { "Type": "date" } } } } );

   var doc = [{ no: 1, tdate: "2034-09-01" },
   { no: 2, tdate: "20345689001" },
   { no: 3, tdate: 2345780 },
   { no: 4, tdate: { "$numberLong": "9223372036854775807" } },
   { no: 5, tdate: 568974.689 },
   { no: 6, tdate: 9222212345678996 },
   { no: 7, tdate: { "$date": "2012-04-05" } },
   { no: 8, tdate: ["2078-12-14", "3456767"] },
   { no: 9, tdate: { "$timestamp": "2023-04-11-13.14.26.124233" } }];
   dbcl.insert( doc );

   // 全文检索，检查结果   
   var esIndexNames = dbOpr.getESIndexNames( COMMCSNAME, clName, indexName );
   var indexRecordNum = 9;
   checkFullSyncToES( COMMCSNAME, clName, indexName, indexRecordNum, esIndexNames );

   var actRecords = dbOpr.findFromCL( dbcl, { "": { "$Text": { query: { "match_all": {} } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   checkResult( doc, actRecords );
}


