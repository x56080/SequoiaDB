/******************************************************************************
 * @Description   : seqDB-31505 :: 创建全文索引，索引字段为double类型
 * @Author        : wu yan 
 * @CreateTime    : 2023.05.11
 * @LastEditTime  : 2023.05.11
 * @LastEditors   : wu yan
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_es_31505";
testConf.clOpt = { ShardingKey: { testdouble: 1 }, ShardingType: "hash", AutoSplit: true };

main( test );
function test ( testPara )
{
   var dbcl = testPara.testCL;
   var clName = testConf.clName;

   var recordNum = 10000;
   var dataGenerator = new commDataGenerator();
   var records = dataGenerator.getRecords( recordNum, "float", ['tdouble'] );
   dbcl.insert( records );

   var indexName = "index_31505";
   dbcl.createIndex( indexName, { "tdouble": "text" } );

   // 全文检索，检查结果   
   var esIndexNames = dbOpr.getESIndexNames( COMMCSNAME, clName, indexName );
   checkFullSyncToES( COMMCSNAME, clName, indexName, recordNum, esIndexNames );

   //var matchConf1 = { testdouble: 1.7E+308 };
   var actRecords = dbOpr.findFromCL( dbcl, { "": { "$Text": { query: { match_all: {} } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expectRecords = dbOpr.findFromCL( dbcl, {}, { _id: { "$include": 0 } }, { _id: 1 } );
   checkResult( expectRecords, actRecords );

   // 创建全文索引后再次插入数据
   var newInsertRecords = [{ no: 1, tdouble: 1.7E+308 }, { no: 2, tdouble: -1.7E+308 }];
   dbcl.insert( newInsertRecords );

   // 检索新插入数据 { no: 1, testdouble: 1.7E+308 }
   checkFullSyncToES( COMMCSNAME, clName, indexName, recordNum + 2, esIndexNames );
   var matchConf = { tdouble: 1.7E+308 };
   actRecords = dbOpr.findFromCL( dbcl, { "": { "$Text": { query: { match: matchConf } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   checkResult( [{ no: 1, tdouble: 1.7E+308 }], actRecords );

   // 删除索引   
   dbcl.dropIndex( indexName );
   checkIndexNotExistInES( esIndexNames );

   //删除新插入记录
   dbcl.remove( newInsertRecords );
}


