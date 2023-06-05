/******************************************************************************
 * @Description   : seqDB-31508 ::  创建全文索引，索引字段为Date类型
 * @Author        : wu yan 
 * @CreateTime    : 2023.05.12
 * @LastEditTime  : 2023.05.12
 * @LastEditors   : wu yan
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_es_31508";

main( test );
function test ( testPara )
{
   var dbcl = testPara.testCL;
   var clName = testConf.clName;
   // 创建全文索引前插入数据
   // var doc = [{ no: 1, tdate: { "$date": "2012-04-05" } },
   // { no: 2, tdate: { "$date": "0000-01-01" } },
   // { no: 3, tdate: { "$date": "2023-05-12" } },
   // { no: 4, tdate: { "$date": "9999-12-31" } },
   // { no: 5, tdate: { "$date": "1970-01-01" } }];
   // dbcl.insert( doc );
   var recordNum = 1000;
   var dataGenerator = new commDataGenerator();
   var records = dataGenerator.getRecords( recordNum, "date", ['tdate'] );
   dbcl.insert( records );

   var indexName = "index_31508";
   dbcl.createIndex( indexName, { "tdate": "text" } );

   // 全文检索，检查结果
   var esIndexNames = dbOpr.getESIndexNames( COMMCSNAME, clName, indexName );
   //var indexRecordNum = 5;
   checkFullSyncToES( COMMCSNAME, clName, indexName, recordNum, esIndexNames );

   //   var matchConf1 = { "tdate": { "lte": "0000-01-01" } };
   var actRecords = dbOpr.findFromCL( dbcl, { "": { "$Text": { query: { "match_all": {} } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expectRecords = dbOpr.findFromCL( dbcl, {}, { _id: { "$include": 0 } }, { _id: 1 } );
   checkResult( expectRecords, actRecords );

   // 创建全文索引后再次插入数据
   var newInsertRecords = [{ no: 1, tdate: { "$date": "0000-01-01" } }, { no: 2, tdate: { "$date": "9999-12-31" } }];
   dbcl.insert( newInsertRecords );

   // 检索新插入数据和旧数据   
   checkFullSyncToES( COMMCSNAME, clName, indexName, recordNum + 2, esIndexNames );
   var matchConf2 = { "tdate": { "lte": "2023-12-31", "gte": "0000-01-01" } };
   var actRecords = dbOpr.findFromCL( dbcl, { "": { "$Text": { query: { "range": matchConf2 } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expectRecords = dbOpr.findFromCL( dbcl, { $and: [{ "tdate": { "$lte": { "$date": "2023-12-31" } } }, { "tdate": { "$gte": { "$date": "0000-01-01" } } }] }, { _id: { "$include": 0 } }, { _id: 1 } );
   checkResult( expectRecords, actRecords );

   // 删除索引   
   dbcl.dropIndex( indexName );
   checkIndexNotExistInES( esIndexNames );

   //删除新插入记录
   dbcl.remove( newInsertRecords );
}


