/******************************************************************************
 * @Description   : seqDB-31587 ::创建全文索引指定映射为date类型，插入索引字段类型为不兼容转换类型
 * @Author        : wu yan 
 * @CreateTime    : 2023.05.19
 * @LastEditTime  : 2023.05.22
 * @LastEditors   : wu yan
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_es_31587";

main( test );
function test ( testPara )
{
   var dbcl = testPara.testCL;
   var clName = testConf.clName;

   var indexName = "index_31587";
   dbcl.createIndex( indexName, { "tdate": "text" }, { "Mappings": { "Fields": { "tdate": { "Type": "date" } } } } );
   var doc = [{ no: 1, tdate: { "city": "shanghai" } },
   { no: 2, tdate: { "city": { "name": "guangzhou" } } },
   { no: 3, tdate: [[12, 34, 56], 789] },
   { no: 4, tdate: [{ "city": "changsha" }] },
   { no: 5, tdate: ["test"] },
   { no: 6, tdate: false }];
   dbcl.insert( doc );

   //全文检索数据，检查结果  
   var esIndexNames = dbOpr.getESIndexNames( COMMCSNAME, clName, indexName );
   checkFullSyncToES( COMMCSNAME, clName, indexName, 0, esIndexNames );
   var actRecords1 = dbOpr.findFromCL( dbcl, { "": { "$Text": { "query": { "match_all": {} } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expectRecords1 = [];
   checkResult( expectRecords1, actRecords1 );

   // 删除索引
   dbcl.dropIndex( indexName );
   checkIndexNotExistInES( esIndexNames );
}


