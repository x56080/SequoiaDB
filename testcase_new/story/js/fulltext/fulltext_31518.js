/******************************************************************************
 * @Description   : seqDB-31518:全文索引字段为array类型，插入索引字段类型为不兼容转换类型
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.05.29
 * @LastEditTime  : 2023.05.29
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "31518";

main( test );
function test ( testPara )
{
   var dbcl = testPara.testCL;
   var clName = testConf.clName;

   // 创建全文索引
   var idxName = "idx_31518";
   dbcl.createIndex( idxName, { "tarr": "text" }, { "Mappings": { "Fields": { "tarr": { "Type": "long" } } } } );

   // 插入数据
   var docs = [
      { no: 1, tarr: { "city": "shanghai" } },
      { no: 2, tarr: ["shanghai", "beijing"] },
      { no: 3, tarr: true },
      { no: 4, tarr: { "$date": "2012-01-01" } },
      { no: 5, tarr: { "$timestamp": "2012-01-01-13.14.26.124233" } }
   ];
   dbcl.insert( docs );

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