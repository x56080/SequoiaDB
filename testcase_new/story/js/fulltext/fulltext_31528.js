/******************************************************************************
 * @Description   : seqDB-31528:多条件组合精确查询
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.05.29
 * @LastEditTime  : 2023.05.29
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_es_31528";

main( test );
function test ( testPara )
{
   var dbcl = testPara.testCL;
   var clName = testConf.clName;

   // 创建全文索引
   var idxName = "idx_31528";
   dbcl.createIndex( idxName, { "tstr": "text", "tdate": "text", "tint": "text" } );

   // 插入数据
   var docs = [
      { tstr: "123", tdate: { "$date": "1970-12-12" }, tint: 2147483647, tobj: { "city": "guangzhou" } },
      { tstr: "123", tdate: { "$date": "2000-01-02" }, tint: 1, tobj: { "city": "guangzhou" } },
      { tstr: "string字符串", tdate: { "$date": "1979-01-01" }, tint: 0, tobj: { a: 1.001 } },
      { tstr: "string", tdate: { "$date": "2023-05-25" }, tint: -2147483648, tobj: { a: 1, b: 2 } }
   ];
   dbcl.insert( docs );

   // 检查全文索引结果
   var dbOpr = new DBOperator();
   var esIndexNames = dbOpr.getESIndexNames( COMMCSNAME, clName, idxName );
   var indexRecordNum = 4;
   checkFullSyncToES( COMMCSNAME, clName, idxName, indexRecordNum, esIndexNames );
   var actRecords1 = dbOpr.findFromCL( dbcl, { "": { "$Text": { query: { "match_all": {} } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expRecords1 = dbOpr.findFromCL( dbcl, {}, { _id: { "$include": 0 } }, { _id: 1 } );
   checkResult( expRecords1, actRecords1 );

   // bool条件查询
   var boolMatch = {
      "bool": {
         "must": { "match": { "tstr": 123 } },
         "should": [{ "match": { "tdate": "1970-12-12" } }, { "match": { "tint": 0 } }],
         "minimum_should_match": 1,
      }
   };
   var actRecords2 = dbOpr.findFromCL( dbcl, { "": { "$Text": { query: boolMatch } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expRecords2 = [{ tstr: "123", tdate: { "$date": "1970-12-12" }, tint: 2147483647, tobj: { "city": "guangzhou" } }];
   checkResult( expRecords2, actRecords2 );

   // dis_max条件查询
   var disMaxMatch = {
      "dis_max": {
         "queries": [
            { "match": { "tstr": "字符串" } },
            { "match": { "tdate": "1970-01-01" } }
         ]
      }
   }
   var actRecords3 = dbOpr.findFromCL( dbcl, { "": { "$Text": { query: disMaxMatch } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expRecords3 = [{ tstr: "string字符串", tdate: { "$date": "1979-01-01" }, tint: 0, tobj: { a: 1.001 } }];
   checkResult( expRecords3, actRecords3 );


   // 删除索引
   dbcl.dropIndex( idxName );
   checkIndexNotExistInES( esIndexNames );
}