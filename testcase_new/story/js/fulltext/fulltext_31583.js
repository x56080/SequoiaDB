/******************************************************************************
 * @Description   : seqDB-31583:创建全文索引指定映射为long类型，插入索引字段类型为不兼容转换类型
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.05.25
 * @LastEditTime  : 2023.05.29
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_es_31583";

main( test );
function test ( testPara )
{
   var dbcl = testPara.testCL;
   var clName = testConf.clName;

   // 插入数据
   var docs = [
      { no: 1, tobj: { a: 1.1 }, tarr: [true, false], tbool: false, tdate: { "$date": "2012-01-01" }, ttime: { "$timestamp": "2012-01-01-13.14.26.124233" } },
      { no: 2147483648, tobj: { a: "1.7e+308" }, tarr: ["test1", "test2"], tbool: true, tdate: { "$date": "0000-01-01" }, ttime: { "$timestamp": "1902-01-01-00.00.00.000000" } },
      { no: -2147483649, tobj: { a: { "$numberLong": "-9223372036854775808" } }, tarr: [2e+308, 2e-308], tbool: false, tdate: { "$date": "9999-12-31" }, ttime: { "$timestamp": "2037-12-31-23.59.59.999999" } },
      { no: 1, tobj: { a: { "$numberLong": "9223372036854775807" } }, tarr: [{ "city": "guangzhou" }, { "city": "shanghai" }], tbool: false, tdate: { "$date": "9999-12-31" }, ttime: { "$timestamp": "2000-12-31-23.59.59.999999" } }
   ];
   dbcl.insert( docs );

   // 创建全文索引，索引字段为double类型
   var idxName = "idx_31583";
   dbcl.createIndex( idxName, {
      no: "text", tobj: "text", tarr: "text", tbool: "text", tdate: "text", ttime: "text"
   }, {
      "Mappings": {
         "Fields": {
            "no": { "Type": "long" }, "tobj": { "Type": "long" }, "tarr": { "Type": "long" },
            "tbool": { "Type": "long" }, "tdate": { "Type": "long" }, "ttime": { "Type": "long" }
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