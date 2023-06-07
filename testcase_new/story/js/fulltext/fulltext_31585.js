/******************************************************************************
 * @Description   : seqDB-31585:创建全文索引指定映射为boolean类型，插入索引字段类型为不兼容转换类型
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.05.26
 * @LastEditTime  : 2023.05.26
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_es_31585";

main( test );
function test ( testPara )
{
   var dbcl = testPara.testCL;
   var clName = testConf.clName;

   // 插入数据
   var docs = [
      { no: 1, tobj: { "city": "guangzhou" } },
      { no: 2, tarr: [1, 2] },
      { no: 3, tstr: "hello" },
      { no: 4, tdate: { "$date": "2012-01-01" } },
      { no: 5, ttime: { "$timestamp": "2012-01-01-13.14.26.124233" } },
      { no: 6, tint: 1 },
      { no: 7, tlong: { "$numberLong": "9223372036854775807" } },
      { no: 8, tdouble: 1.1 },
      { no: 9, tfloat: 1.1 },
   ];
   dbcl.insert( docs );

   // 创建全文索引，索引字段为boolean类型
   var idxName = "idx_31585";
   dbcl.createIndex( idxName, {
      tobj: "text", tstr: "text", tarr: "text", tstr: "text", tdate: "text",
      ttime: "text", tint: "text", tlong: "text", tdouble: "text", tfloat: "text"
   }, {
      "Mappings": {
         "Fields": {
            "tobj": { "Type": "boolean" }, "tstr": { "Type": "boolean" }, "tarr": { "Type": "boolean" },
            "tdate": { "Type": "boolean" }, "ttime": { "Type": "boolean" }, "tint": { "Type": "boolean" },
            "tlong": { "Type": "boolean" }, "tdouble": { "Type": "boolean" }, "tfloat": { "Type": "boolean" }
         }
      }
   } );

   // 检查全文索引结果
   var dbOpr = new DBOperator();
   var esIndexNames = dbOpr.getESIndexNames( COMMCSNAME, clName, idxName );
   var indexRecordNum = 0
   checkFullSyncToES( COMMCSNAME, clName, idxName, indexRecordNum, esIndexNames );
   var actRecords1 = dbOpr.findFromCL( dbcl, { "": { "$Text": { query: { "match_all": {} } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expectRecords1 = [];
   checkResult( expectRecords1, actRecords1 );

   // 删除索引
   dbcl.dropIndex( idxName );
   checkIndexNotExistInES( esIndexNames );
}