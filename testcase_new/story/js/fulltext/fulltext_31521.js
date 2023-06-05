/******************************************************************************
 * @Description   : seqDB-31521 :: 切分表创建全文索引，设置映射类型
 * @Author        : wu yan 
 * @CreateTime    : 2023.05.22
 * @LastEditTime  : 2023.05.22
 * @LastEditors   : wu yan
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipOneGroup = true;
testConf.useSrcGroup = true;
testConf.useDstGroup = true;
testConf.clName = COMMCLNAME + "_es_31521";
testConf.clOpt = { ShardingKey: { tdate: 1 }, ShardingType: "range" };

main( test );
function test ( testPara )
{
   var dbcl = testPara.testCL;
   var clName = testConf.clName;
   var indexName = "index_31521";
   var mappingsOption = { "Fields": { "tdate": { "Type": "keyword" } } };
   dbcl.createIndex( indexName, { tdate: "text" }, { "Mappings": mappingsOption } );
   snapshotIndexCheckMappings( dbcl, indexName, mappingsOption );

   var doc = [{ no: 1, tdate: { "$date": "2001-01-01" } },
   { no: 2, tdate: { "$date": "0000-01-01" } },
   { no: 3, tdate: { "$date": "2100-11-01" } },
   { no: 4, tdate: { "$date": "1980-01-01" } },
   { no: 5, tdate: { "$date": "2021-01-11" } },
   { no: 6, tdate: { "$date": "2007-11-30" } },
   { no: 7, tdate: { "$date": "2024-05-01" } },
   { no: 8, tdate: { "$date": "2009-08-10" } },
   { no: 9, tdate: { "$date": "9999-02-01" } },
   { no: 10, tdate: { "$date": "3045-07-05" } }];
   dbcl.insert( doc );
   dbcl.split( testPara.srcGroupName, testPara.dstGroupNames[0], 60 );

   //全文检索数据，检查结果  
   var esIndexNames = dbOpr.getESIndexNames( COMMCSNAME, clName, indexName );
   checkFullSyncToES( COMMCSNAME, clName, indexName, 10, esIndexNames );
   var actRecords1 = dbOpr.findFromCL( dbcl, { "": { "$Text": { "query": { "match_all": {} } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   checkResult( doc, actRecords1 );

   //检索范围覆盖多个数据组
   var actRecords2 = dbOpr.findFromCL( dbcl, { "": { "$Text": { "query": { "range": { "tdate": { "gte": "1980-01-01", "lte": "2024-05-01" } } } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expectRecords2 = dbOpr.findFromCL( dbcl, { "$and": [{ "tdate": { "$gte": { "$date": "1980-01-01" } } }, { "tdate": { "$lte": { "$date": "2024-05-01" } } }] }, { _id: { "$include": 0 } }, { _id: 1 } );
   checkResult( expectRecords2, actRecords2 );

   // 删除索引
   dbcl.dropIndex( indexName );
   checkIndexNotExistInES( esIndexNames );
}


