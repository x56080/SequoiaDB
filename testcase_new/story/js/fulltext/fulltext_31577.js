/******************************************************************************
 * @Description   : seqDB-31577:创建全文索引指定映射为integer类型，插入索引字段类型为不兼容转换类型
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.05.23
 * @LastEditTime  : 2023.05.25
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_es_31577";

main( test );
function test ( testPara )
{
   var dbcl = testPara.testCL;
   var clName = testConf.clName;

   // 插入数据
   var docs = [
      { no: 1, tobj: { "num": 0 }, tarr: ["12", "34"], tbool: true, tdate: { "$date": "2012-01-01" }, ttime: { "$timestamp": "2012-01-01-13.14.26.124233" }, tlong: 3000000000, tdouble: -1.7E+308, tfloat: 1.7E+308 },
      { no: 2147483648, tobj: { "num": 2147483648 }, tarr: [2147483648, 0], tbool: false, tdate: { "$date": "0000-01-01" }, ttime: { "$timestamp": "1902-01-01-00.00.00.000000" }, tlong: { "$numberLong": "-9223372036854775808" }, tdouble: 1.7E+308, tfloat: -1.7E+308 },
      { no: -2147483649, tobj: { "num": -2147483649 }, tarr: [-2147483649, 0], tbool: false, tdate: { "$date": "9999-12-31" }, ttime: { "$timestamp": "2037-12-31-23.59.59.999999" }, tlong: { "$numberLong": "9223372036854775807" }, tdouble: -0.123, tfloat: 5000.46 },
      { no: -1, tobj: { "num": -1 }, tarr: [true, false], tbool: true, tdate: { "$date": "1970-01-01" }, ttime: { "$timestamp": "2022-01-01-03.14.26.124233" }, tlong: { "$numberLong": "-1" }, tdouble: 2147483648.9, tfloat: -2147483649.9 }
   ];
   dbcl.insert( docs );

   // 创建全文索引指定映射为integer类型
   var idxName = "idx_31577";
   dbcl.createIndex( idxName, {
      no: "text", tobj: "text", tarr: "text", tbool: "text", tdate: "text", ttime: "text", tlong: "text", tdouble: "text", tfloat: "text"
   }, {
      "Mappings": {
         "Fields": {
            "no": { "Type": "integer" }, "tobj": { "Type": "integer" }, "tarr": { "Type": "integer" },
            "tbool": { "Type": "integer" }, "tdate": { "Type": "integer" }, "ttime": { "Type": "integer" },
            "tlong": { "Type": "integer" }, "tdouble": { "Type": "integer" }, "tfloat": { "Type": "integer" }
         }
      }
   } );

   // 检查全文索引结果
   var dbOpr = new DBOperator();
   var esIndexNames = dbOpr.getESIndexNames( COMMCSNAME, clName, idxName );
   var indexRecordNum = 0;
   checkFullSyncToES( COMMCSNAME, clName, idxName, indexRecordNum, esIndexNames );
   var actRecords1 = dbOpr.findFromCL( dbcl, { "": { "$Text": { query: { "match_all": {} } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expectRecords1 = [];
   checkResult( expectRecords1, actRecords1 );

   // 删除索引
   dbcl.dropIndex( idxName );
   checkIndexNotExistInES( esIndexNames );
}