/******************************************************************************
 * @Description   : seqDB-31512:创建全文索引，索引字段为不符合默认映射类型
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.05.29
 * @LastEditTime  : 2023.05.29
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_es_31512";

main( test );
function test ( testPara )
{
   var dbcl = testPara.testCL;
   var clName = testConf.clName;

   // 插入数据
   var docs = [
      { no: 1, a: { "$oid": "123abcd00ef12358902300ef" } },
      { no: 2, a: { $decimal: "123.456" } },
      { no: 3, a: { "$minKey": 1 } },
      { no: 4, a: { "$maxKey": 1 } },
      { no: 5, a: null },
      { no: 6, a: undefined },
      { no: 7, a: { $regex: "^z", "$options": "i" } },
      { no: 8, a: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" } }
   ]
   dbcl.insert( docs );

   // 创建全文索引
   var idxName = "idx_31512";
   dbcl.createIndex( idxName, { "a": "text" } );

   // 检查全文索引结果
   var dbOpr = new DBOperator();
   var esIndexNames = dbOpr.getESIndexNames( COMMCSNAME, clName, idxName );
   var indexRecordNum = 0;
   checkFullSyncToES( COMMCSNAME, clName, idxName, indexRecordNum, esIndexNames );

   // 指定范围匹配索引字段值
   var actRecords = dbOpr.findFromCL( dbcl, { "": { "$Text": { query: { "match_all": {} } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expectRecords = [];
   checkResult( expectRecords, actRecords );

   // 删除索引
   dbcl.dropIndex( idxName );
   checkIndexNotExistInES( esIndexNames );
}