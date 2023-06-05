/******************************************************************************
 * @Description   :  seqDB-31506 :: 创建全文索引，索引字段为numberLong类型
 * @Author        : wu yan 
 * @CreateTime    : 2023.05.11
 * @LastEditTime  : 2023.05.11
 * @LastEditors   : wu yan
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_es_31506";
testConf.clOpt = { ShardingKey: { no: 1 }, ShardingType: "hash", AutoSplit: true };

main( test );
function test ( testPara )
{
   var dbcl = testPara.testCL;
   var clName = testConf.clName;
   // 创建全文索引前插入数据
   var doc = [{ no: 1, tlong: { "$numberLong": "9223372036854775807" } },
   { no: 2, tlong: 3000000000 },
   { no: 3, tlong: { "$numberLong": "-270000" } },
   { no: 4, tlong: { "$numberLong": "-9223372036854775808" } }];
   dbcl.insert( doc );

   var indexName = "index_31506";
   dbcl.createIndex( indexName, { "tlong": "text" } );

   // 全文检索，检查结果   
   var esIndexNames = dbOpr.getESIndexNames( COMMCSNAME, clName, indexName );
   var indexRecordNum = 4;
   checkFullSyncToES( COMMCSNAME, clName, indexName, indexRecordNum, esIndexNames );

   var matchConf1 = { tlong: { "$numberLong": "-9223372036854775808" } };
   var actRecords = dbOpr.findFromCL( dbcl, { "": { "$Text": { query: { match: matchConf1 } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expectRecords = [{ no: 4, tlong: { "$numberLong": "-9223372036854775808" } }];
   checkResult( expectRecords, actRecords );

   // 创建全文索引后再次插入数据
   var newInsertRecords = { no: 5, tlong: -3000000000 };
   dbcl.insert( newInsertRecords );

   // 全文检索，检查结果
   indexRecordNum = 5;
   checkFullSyncToES( COMMCSNAME, clName, indexName, indexRecordNum, esIndexNames );
   var matchConf2 = { "tlong": -3000000000 };
   actRecords = dbOpr.findFromCL( dbcl, { "": { "$Text": { query: { match: matchConf2 } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   checkResult( [newInsertRecords], actRecords );

   // 删除索引   
   dbcl.dropIndex( indexName );
   checkIndexNotExistInES( esIndexNames );

   //删除新插入记录
   dbcl.remove( newInsertRecords );
}


