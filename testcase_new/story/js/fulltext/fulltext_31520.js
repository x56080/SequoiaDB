/******************************************************************************
 * @Description   : seqDB-31520:切分表创建全文索引，不设置映射类型
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.05.29
 * @LastEditTime  : 2023.05.29
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipOneGroup = true;
testConf.useSrcGroup = true;
testConf.useDstGroup = true;
testConf.clName = COMMCLNAME + "_es_31520";
testConf.clOpt = { ShardingKey: { no: 1 }, ShardingType: "range" };

main( test );
function test ( testPara)
{
   dbcl = testPara.testCL;
   clName = testConf.clName;

   // 创建全文索引
   var idxName = "idx_31520";
   dbcl.createIndex( idxName, { "tsplit": "text" } );

   // 插入数据
   var docs = [
      { no: 1, tsplit: "hello world" },
      { no: 2, tsplit: ["test1", "test2"] },
      { no: 3, tsplit: true, tstr: "true", tbool: true },
      { no: 4, tsplit: { "$timestamp": "2012-01-01-13.14.26.124233" }, tlong: { "$numberLong": "-2147483648" } },
   ];
   dbcl.insert( docs );
   dbcl.split( testPara.srcGroupName, testPara.dstGroupNames[0], 60 );

   // 检查全文索引结果
   var dbOpr = new DBOperator();
   var esIndexNames = dbOpr.getESIndexNames( COMMCSNAME, clName, idxName );
   var indexRecordNum = 4;
   checkFullSyncToES( COMMCSNAME, clName, idxName, indexRecordNum, esIndexNames );
   var actRecords1 = dbOpr.findFromCL( dbcl, { "": { "$Text": { query: { "match_all": {} } } } }, { no: { "$include": 0 } }, { no: 1 } );
   var expectRecords1 = dbOpr.findFromCL( dbcl, {}, { no: { "$include": 0 } }, { no: 1 } );
   checkResult( expectRecords1, actRecords1 );

   // 删除索引
   dbcl.dropIndex( idxName );
   checkIndexNotExistInES( esIndexNames );
}