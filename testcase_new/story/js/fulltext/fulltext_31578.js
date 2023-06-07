/******************************************************************************
 * @Description   : seqDB-31578:创建全文索引指定映射为double类型，插入索引字段类型为兼容转换类型
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.05.25
 * @LastEditTime  : 2023.05.25
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_es_31578";

main( test );
function test ( testPara )
{
   var dbcl = testPara.testCL;
   var clName = testConf.clName;

   // 插入数据
   var docs = [
      { no: 1, tstr: "123.456", tlong: { "$numberLong": "123" }, tdouble: 123.456, tfloat: 123.456, tarr: [123, 123.456, -123.456] },
      { no: 2147483647, tstr: "1.7e+308", tlong: { "$numberLong": "2147483647" }, tdouble: 1.7e+308, tfloat: 1.7e+308, tarr: [1.7e+308, 0, 2147483647] },
      { no: -2147483648, tstr: "-1.7e+308", tlong: { "$numberLong": "-2147483648" }, tdouble: -1.7e+308, tarr: [-1.7e+308, 0, -12147483648] },
      { no: -1, tstr: "-1", tlong: { "$numberLong": "9223372036854775807" }, tdouble: -0.01, tarr: [-0.01, 0.01, 1.7e+308, -1.7e+308] }
   ];
   dbcl.insert( docs );

   // 创建全文索引，索引字段为double类型
   var idxName = "idx_31578";
   dbcl.createIndex( idxName, {
      no: "text", tstr: "text", tlong: "text", tdouble: "text", tfloat: "text", tarr: "text"
   }, {
      "Mappings": {
         "Fields": {
            "no": { "Type": "double" }, "tstr": { "Type": "double" }, "tlong": { "Type": "double" },
            "tdouble": { "Type": "double" }, "tfloat": { "Type": "double" }, "tarr": { "Type": "double" }
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
   var actRecords3 = dbOpr.findFromCL( dbcl, { "": { "$Text": { query: { "match": { "tstr": "1.7e+308" } } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expectRecords3 = dbOpr.findFromCL( dbcl, { "tstr": "1.7e+308" }, { _id: { "$include": 0 } }, { _id: 1 } );
   checkResult( expectRecords3, actRecords3 );

   // 检查long类型字段
   var actRecords4 = dbOpr.findFromCL( dbcl, { "": { "$Text": { query: { "match": { "tlong": { "$numberLong": "9223372036854775807" } } } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expectRecords4 = dbOpr.findFromCL( dbcl, { "tlong": { "$numberLong": "9223372036854775807" } }, { _id: { "$include": 0 } }, { _id: 1 } );
   checkResult( expectRecords4, actRecords4 );

   // 检查double类型字段
   var actRecords5 = dbOpr.findFromCL( dbcl, { "": { "$Text": { query: { "match": { "tdouble": 1.7e+308 } } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expectRecords5 = dbOpr.findFromCL( dbcl, { "tdouble": 1.7e+308 }, { _id: { "$include": 0 } }, { _id: 1 } );
   checkResult( expectRecords5, actRecords5 );

   // 检查float类型字段
   var actRecords6 = dbOpr.findFromCL( dbcl, { "": { "$Text": { query: { "match": { "tfloat": 1.7e+308 } } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expectRecords6 = dbOpr.findFromCL( dbcl, { "tfloat": 1.7e+308 }, { _id: { "$include": 0 } }, { _id: 1 } );
   checkResult( expectRecords6, actRecords6 );

   // 检查array类型字段
   var actRecords7 = dbOpr.findFromCL( dbcl, { "": { "$Text": { query: { "match": { "tarr": -0.01 } } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expectRecords7 = dbOpr.findFromCL( dbcl, { "tarr": -0.01 }, { _id: { "$include": 0 } }, { _id: 1 } );
   checkResult( expectRecords7, actRecords7 );

   //再次插入索引数据，检索新插入的数据
   var newInsertRecords = { no: 0, tstr: "0", tlong: { "$numberLong": "0" }, tdouble: 0.001, tfloat: 0, tarr: [0, 0, 0] };
   dbcl.insert( newInsertRecords );
   checkFullSyncToES( COMMCSNAME, clName, idxName, indexRecordNum + 1, esIndexNames );
   var actRecords8 = dbOpr.findFromCL( dbcl, { "": { "$Text": { query: { "match_all": {} } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expectRecords8 = dbOpr.findFromCL( dbcl, {}, { _id: { "$include": 0 } }, { _id: 1 } );
   checkResult( expectRecords8, actRecords8 );

   // 删除索引
   dbcl.dropIndex( idxName );
   checkIndexNotExistInES( esIndexNames );
}   