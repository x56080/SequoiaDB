/******************************************************************************
 * @Description   : seqDB-31576:创建全文索引指定映射为integer类型，插入索引字段类型为兼容转换类型
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.05.29
 * @LastEditTime  : 2023.05.29
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_es_31576";

main( test );
function test ( testPara )
{
   var dbcl = testPara.testCL;
   var clName = testConf.clName;

   // 插入数据
   var docs = [
      { no: 1, tstr: "123", tlong: { "$numberLong": "123" }, tdouble: 123.456, tarr: [123, 456, 789] },
      { no: 2147483647, tstr: "2147483647", tlong: { "$numberLong": "2147483647" }, tarr: [2147483647, 0] },
      { no: -2147483648, tstr: "-2147483648", tlong: { "$numberLong": "-2147483648" }, tarr: [-12147483648, 0] },
      { no: -1, tstr: "-1", tlong: { "$numberLong": "2147483647" }, tarr: [-1, 1, 0, 2] }
   ];
   dbcl.insert( docs );

   // 创建全文索引指定映射为integer类型
   var idxName = "idx_31576";
   dbcl.createIndex( idxName, {
      no: "text", tstr: "text", tlong: "text", tdouble: "text", tarr: "text"
   }, {
      "Mappings": {
         "Fields": {
            "no": { "Type": "integer" }, "tstr": { "Type": "integer" }, "tlong": { "Type": "integer" },
            "tdouble": { "Type": "integer" }, "tarr": { "Type": "integer" }
         }
      }
   } );

   // 检查全文索引结果
   var dbOpr = new DBOperator();
   var esIndexNames = dbOpr.getESIndexNames( COMMCSNAME, clName, idxName );
   var indexRecordNum = 4;
   checkFullSyncToES( COMMCSNAME, clName, idxName, indexRecordNum, esIndexNames );
   var actRecords1 = dbOpr.findFromCL( dbcl, { "": { "$Text": { query: { "match_all": {} } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expectRecords1 = dbOpr.findFromCL( dbcl, {}, { _id: { "$include": 0 } }, { _id: 1 } );
   checkResult( expectRecords1, actRecords1 );

   // 检查integer类型字段
   var actRecords2 = dbOpr.findFromCL( dbcl, { "": { "$Text": { query: { "match": { "no": 2147483647 } } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expectRecords2 = dbOpr.findFromCL( dbcl, { "no": 2147483647 }, { _id: { "$include": 0 } }, { _id: 1 } );
   checkResult( expectRecords2, actRecords2 );

   // 检查string类型字段
   var actRecords3 = dbOpr.findFromCL( dbcl, { "": { "$Text": { query: { "match": { "tstr": 123 } } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expectRecords3 = dbOpr.findFromCL( dbcl, { "tstr": "123" }, { _id: { "$include": 0 } }, { _id: 1 } );
   checkResult( expectRecords3, actRecords3 );

   // 检查long类型字段
   var actRecords4 = dbOpr.findFromCL( dbcl, { "": { "$Text": { query: { "match": { "tlong": 2147483647 } } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expectRecords4 = dbOpr.findFromCL( dbcl, { "tlong": { "$numberLong": "2147483647" } }, { _id: { "$include": 0 } }, { _id: 1 } );
   checkResult( expectRecords4, actRecords4 );

   // 检查double类型字段
   var actRecords5 = dbOpr.findFromCL( dbcl, { "": { "$Text": { query: { "match": { "tdouble": 123 } } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expectRecords5 = dbOpr.findFromCL( dbcl, { "tdouble": 123.456 }, { _id: { "$include": 0 } }, { _id: 1 } );
   checkResult( expectRecords5, actRecords5 );

   // 检查array类型字段
   var actRecords6 = dbOpr.findFromCL( dbcl, { "": { "$Text": { query: { "match": { "tarr": 123 } } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expectRecords6 = dbOpr.findFromCL( dbcl, { "tarr": { "$elemMatch": { "$et": 123 } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   checkResult( expectRecords6, actRecords6 );

   //再次插入索引数据，检索新插入的数据
   var newInsertRecords = { no: 7, tstr: "0", tlong: { "$numberLong": "0" }, tdouble: 0, tarr: [0, 12, -1] };
   dbcl.insert( newInsertRecords );
   checkFullSyncToES( COMMCSNAME, clName, idxName, indexRecordNum + 1, esIndexNames );
   var actRecords7 = dbOpr.findFromCL( dbcl, { "": { "$Text": { query: { "match_all": {} } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expectRecords7 = dbOpr.findFromCL( dbcl, {}, { _id: { "$include": 0 } }, { _id: 1 } );
   checkResult( expectRecords7, actRecords7 );

   // 删除索引
   dbcl.dropIndex( idxName );
   checkIndexNotExistInES( esIndexNames );
}