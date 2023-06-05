/******************************************************************************
 * @Description   : seqDB-31575 :: 创建全文索引指定映射为wildcard类型，插入索引字段类型为不兼容转换类型
 * @Author        : wu yan 
 * @CreateTime    : 2023.05.17
 * @LastEditTime  : 2023.05.17
 * @LastEditors   : wu yan
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_es_31575";

main( test );
function test ( testPara )
{
   var dbcl = testPara.testCL;
   var clName = testConf.clName;

   var indexName = "index_31575";
   dbcl.createIndex( indexName, { tobj: "text", tarr: "text" }, { "Mappings": { "Fields": { "tobj": { "Type": "wildcard" }, "tarr": { "Type": "wildcard" } } } } );
   var doc = [{ no: 1, tobj: { "city": "shanghai" }, tarr: ["test1", 123] },
   { no: 2, tobj: { "city": { "name": "guangzhou" } }, tarr: [{ no: 12 }, { no: 3456 }] },
   { no: 3, tobj: { "city": "shenzhen" }, tarr: [[12, 34, 56], 789] },
   { no: 4, tobj: { "city": "changsha" }, tarr: [["testoa", "testb"]] }];
   dbcl.insert( doc );

   //全文检索数据，检查结果  
   var esIndexNames = dbOpr.getESIndexNames( COMMCSNAME, clName, indexName );
   checkFullSyncToES( COMMCSNAME, clName, indexName, 0, esIndexNames );
   var actRecords1 = dbOpr.findFromCL( dbcl, { "": { "$Text": { "query": { "match_all": {} } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expectRecords1 = [];
   checkResult( expectRecords1, actRecords1 );

   //检索object类型数据  
   var actRecords2 = dbOpr.findFromCL( dbcl, { "": { "$Text": { "query": { "wildcard": { "tobj.city": "shanghai" } } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expectRecords2 = [];
   checkResult( expectRecords2, actRecords2 );

   //检索array类型数据
   var actRecords3 = dbOpr.findFromCL( dbcl, { "": { "$Text": { "query": { "wildcard": { "tarr": "test1" } } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expectRecords3 = [];
   checkResult( expectRecords3, actRecords3 );

   // 删除索引
   dbcl.dropIndex( indexName );
   checkIndexNotExistInES( esIndexNames );
}


