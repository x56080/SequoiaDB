/******************************************************************************
 * @Description   : seqDB-31619:全文索引字段为object类型，插入索引字段类型为不兼容转换类型
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.06.05
 * @LastEditTime  : 2023.06.05
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_es_31619";

main( test );
function test ( testPara )
{
   var dbcl = testPara.testCL;
   var clName = testConf.clName;

   // 创建全文索引
   dbcl.insert( { no: 0, tobj: { "test": { "$numberLong": "9223372036854775807" } } } );
   var idxName = "idx_31619";
   dbcl.createIndex( idxName, { "tobj": "text" } );

   // 插入数据
   var docs = [
      { no: 1, tobj: { "test": { "$date": "2023-05-25" } } },
      { no: 2, tobj: { "test": [ "test1", "test2" ] } },
      { no: 3, tobj: { "test": { "$timestamp": "2012-01-01-13.14.26.124233" } } },
      { no: 4, tobj: { "test": true } }
   ];
   dbcl.insert( docs );

   // 检查全文索引结果
   var dbOpr = new DBOperator();
   var esIndexNames = dbOpr.getESIndexNames( COMMCSNAME, clName, idxName );
   var indexRecordNum = 1;
   checkFullSyncToES( COMMCSNAME, clName, idxName, indexRecordNum, esIndexNames );
   var actRecords1 = dbOpr.findFromCL( dbcl, { "": { "$Text": { query: { "match_all": {} } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expectRecords1 = [ { no: 0, tobj: { "test": { "$numberLong": "9223372036854775807" } } } ];
   checkResult( expectRecords1, actRecords1 );

   // 删除索引
   dbcl.dropIndex( idxName );
   checkIndexNotExistInES( esIndexNames );
}
