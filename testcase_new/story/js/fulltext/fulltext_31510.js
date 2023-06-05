/******************************************************************************
 * @Description   : seqDB-31510 :: 创建全文索引，索引字段为Object类型
 * @Author        : wu yan 
 * @CreateTime    : 2023.05.12
 * @LastEditTime  : 2023.05.12
 * @LastEditors   : wu yan
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_es_31510";
testConf.clOpt = { ShardingKey: { no: 1 }, ShardingType: "hash", AutoSplit: true };

main( test );
function test ( testPara )
{
   var dbcl = testPara.testCL;
   var clName = testConf.clName;
   // 创建全文索引前插入数据
   var doc = [{ no: 1, tobj: { "city": "beijing", "num": 1 }, tobj1: { "city": { "name": "beijing", "num": 1 } }, tobj2: { "per": { "city": { "name": "beijing" } } } },
   { no: 2, tobj: { "city": "shanghai" }, tobj1: { "city": { "name": "shanghai", "num": 2 } }, tobj2: { "per": { "city": { "name": "wuhan" } } } },
   { no: 3, tobj: { "city": "wuhan" }, tobj1: { "city": { "name": "wuhan", "num": 3 } }, tobj2: { "per": { "city": { "name": "changsha" } } } },
   { no: 4, tobj: { "city": "shenzhen" }, tobj1: { "city": { "name": "shenzhen", "num": 4 } }, tobj2: { "per": { "city": { "name": "haerbin" } } } },
   { no: 5, tobj: { "city": "guangzhou01" }, tobj1: { "city": { "name": "guangzhou", "num": 5 } }, tobj2: { "per": { "city": { "name": "shenzhen" } } } }];
   dbcl.insert( doc );

   var indexName = "index_31510";
   dbcl.createIndex( indexName, { "tobj": "text" } );

   //对象类型创建全文索引，执行全文检索   
   var indexRecordNum = 5;
   checkFullSyncToES( COMMCSNAME, clName, indexName, indexRecordNum, esIndexNames );

   var matchConf1 = { "tobj.num": 1 };
   var actRecords1 = dbOpr.findFromCL( dbcl, { "": { "$Text": { query: { match: matchConf1 } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expectRecords1 = dbOpr.findFromCL( dbcl, matchConf1, { _id: { "$include": 0 } }, { _id: 1 } );

   checkResult( expectRecords1, actRecords1 );

   //一层嵌套对象类型创建全文索引，执行检索
   dbcl.dropIndex( indexName );
   dbcl.createIndex( indexName, { "tobj1": "text" } );
   var matchConf2 = { "tobj1.city.name": "shanghai" };
   checkFullSyncToES( COMMCSNAME, clName, indexName, indexRecordNum );
   var expectRecords2 = dbOpr.findFromCL( dbcl, matchConf2, { _id: { "$include": 0 } }, { _id: 1 } );
   var actRecords2 = dbOpr.findFromCL( dbcl, { "": { "$Text": { query: { match: matchConf2 } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   checkResult( expectRecords2, actRecords2 );

   //多层嵌套对象类型创建全文索引，执行检索
   dbcl.dropIndex( indexName );
   dbcl.createIndex( indexName, { "tobj2": "text" } );
   var esIndexNames = dbOpr.getESIndexNames( COMMCSNAME, clName, indexName );
   var matchConf3 = { "tobj2.per.city.name": "shenzhen" };
   checkFullSyncToES( COMMCSNAME, clName, indexName, indexRecordNum, esIndexNames );
   var actRecords3 = dbOpr.findFromCL( dbcl, { "": { "$Text": { query: { match: matchConf3 } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expectRecords3 = dbOpr.findFromCL( dbcl, matchConf3, { _id: { "$include": 0 } }, { _id: 1 } );;
   checkResult( expectRecords3, actRecords3 );

   // 删除索引
   dbcl.dropIndex( indexName );
   checkIndexNotExistInES( esIndexNames );
}


