/******************************************************************************
 * @Description   : seqDB-31588:创建全文索引，多个索引字段指定映射不同类型
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.05.22
 * @LastEditTime  : 2023.05.29
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_es_31588";

main( test );
function test ( testPara )
{
   var dbcl = testPara.testCL;
   var clName = testConf.clName;

   // 创建全文索引
   var idxName = "idx_31588";
   var mappingsInfo = { 
        "Fields": { 
            "tstr": { "Type": "text" }, "tdate": { "Type": "date" }, "tint": { "Type": "integer" }
        } 
   };
   dbcl.createIndex( idxName, {
      "tstr": "text", "tdate": "text", "tint": "text", "tobj": "text"
   }, { "Mappings": mappingsInfo } );
   listIndexCheckMappings( dbcl, idxName, mappingsInfo );   
   snapshotIndexCheckMappings( dbcl, idxName, mappingsInfo );

   // 插入数据
   var docs = [
      { tstr: "123", tdate: { "$date": "1970-01-01" }, tint: 2147483647, tobj: { a: 1, b: 2 } },
      { tstr: "abc", tdate: { "$date": "2020-12-12" }, tint: 0, tobj: { a: 1.001 } },
      { tstr: "字符串", tdate: { "$date": "2023-05-25" }, tint: -2147483648, tobj: { "city": "guangzhou" } }
   ];
   dbcl.insert( docs );

   // 检查全文索引结果
   var dbOpr = new DBOperator();
   var esIndexNames = dbOpr.getESIndexNames( COMMCSNAME, clName, idxName );
   var indexRecordNum = 3;
   checkFullSyncToES( COMMCSNAME, clName, idxName, indexRecordNum, esIndexNames );
   var actRecords1 = dbOpr.findFromCL( dbcl, { "": { "$Text": { query: { "match_all": {} } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expectRecords1 = dbOpr.findFromCL( dbcl, {}, { _id: { "$include": 0 } }, { _id: 1 } );
   checkResult( expectRecords1, actRecords1 );

   // 检查string类型字段
   var actRecords2 = dbOpr.findFromCL( dbcl, { "": { "$Text": { query: { "match": { "tstr": "字符串" } } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expectRecords2 = dbOpr.findFromCL( dbcl, { "tstr": "字符串" }, { _id: { "$include": 0 } }, { _id: 1 } );
   checkResult( expectRecords2, actRecords2 );

   // 检查date类型字段
   var actRecords3 = dbOpr.findFromCL( dbcl, { "": { "$Text": { query: { "match": { "tdate": "2023-05-25" } } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expectRecords3 = dbOpr.findFromCL( dbcl, { "tdate": { "$date": "2023-05-25" } }, { _id: { "$include": 0 } }, { _id: 1 } );
   checkResult( expectRecords3, actRecords3 );

   // 检查int类型字段
   var actRecords4 = dbOpr.findFromCL( dbcl, { "": { "$Text": { query: { "match": { "tint": "2147483647" } } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expectRecords4 = dbOpr.findFromCL( dbcl, { "tint": 2147483647 }, { _id: { "$include": 0 } }, { _id: 1 } );
   checkResult( expectRecords4, actRecords4 );

   // 检查object类型字段
   var actRecords5 = dbOpr.findFromCL( dbcl, { "": { "$Text": { query: { "match": { "tobj.city": "guangzhou" } } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expectRecords5 = dbOpr.findFromCL( dbcl, { "tobj.city": "guangzhou" }, { _id: { "$include": 0 } }, { _id: 1 } );
   checkResult( expectRecords5, actRecords5 );

   // 删除索引
   dbcl.dropIndex( idxName );
   checkIndexNotExistInES( esIndexNames );
}
