/******************************************************************************
 * @Description   : seqDB-31620:全文索引字段为object类型，插入索引字段类型为兼容转换类型
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.06.05
 * @LastEditTime  : 2023.06.05
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_es_31620";

main( test );
function test ( testPara )
{
   var dbcl = testPara.testCL;
   var clName = testConf.clName;

   // 创建全文索引
   var idxName = "idx_31620";
   dbcl.createIndex( idxName, { "tobj": "text" } );

   // 插入数据
   var docs = [
      { no: 1, tobj: { "test": "guangzhou" } },
      { no: 2, tobj: { "test": 2147483647 } },
      { no: 3, tobj: { "test": { "$numberLong": "123" } } },
      { no: 4, tobj: { "test": 123.456 } },
      { no: 5, tobj: { "test": -3.4e+38 } },
      { no: 6, tobj: { "test": true } },
      { no: 7, tobj: { "test": { "$date": "2023-05-25" } } },
      { no: 8, tobj: { "test": [123, 456, 789] } }
   ];
   dbcl.insert( docs );

   // 检查全文索引结果
   var dbOpr = new DBOperator();
   var esIndexNames = dbOpr.getESIndexNames( COMMCSNAME, clName, idxName );
   var indexRecordNum = 8;
   checkFullSyncToES( COMMCSNAME, clName, idxName, indexRecordNum, esIndexNames );
   var actRecords1 = dbOpr.findFromCL( dbcl, { "": { "$Text": { query: { "match_all": {} } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expectRecords1 = dbOpr.findFromCL( dbcl, {}, { _id: { "$include": 0 } }, { _id: 1 } );
   checkResult( expectRecords1, actRecords1 );

   // 检查string类型字段
   var actRecords2 = dbOpr.findFromCL( dbcl, { "": { "$Text": { query: { "match": { "tobj.test": "guangzhou" } } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expectRecords2 = dbOpr.findFromCL( dbcl, { "tobj.test": "guangzhou" }, { _id: { "$include": 0 } }, { _id: 1 } );
   checkResult( expectRecords2, actRecords2 );

   // 检查int类型字段
   var actRecords3 = dbOpr.findFromCL( dbcl, { "": { "$Text": { query: { "match": { "tobj.test": "2147483647" } } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expRecords3 = dbOpr.findFromCL( dbcl, { "tobj.test": 2147483647 }, { _id: { "$include": 0 } }, { _id: 1 } );
   checkResult( expRecords3, actRecords3 );

   // 检查long类型字段
   var actRecords4 = dbOpr.findFromCL( dbcl, { "": { "$Text": { query: { "match": { "tobj.test": { "$numberLong": "123" } } } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expRecords4 = dbOpr.findFromCL( dbcl, { "tobj.test": { "$numberLong": "123" } }, { _id: { "$include": 0 } }, { _id: 1 } );
   checkResult( expRecords4, actRecords4 );

   // 检查double类型字段
   var actRecords5 = dbOpr.findFromCL( dbcl, { "": { "$Text": { query: { "match": { "tobj.test": 123.456 } } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expRecords5 = dbOpr.findFromCL( dbcl, { "tobj.test": 123.456 }, { _id: { "$include": 0 } }, { _id: 1 } );
   checkResult( expRecords5, actRecords5 );

   // 检查float类型字段
   var actRecords6 = dbOpr.findFromCL( dbcl, { "": { "$Text": { query: { "match": { "tobj.test": "-3.4E+38" } } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expRecords6 = dbOpr.findFromCL( dbcl, { "tobj.test": -3.4e+38 }, { _id: { "$include": 0 } }, { _id: 1 } );
   checkResult( expRecords6, actRecords6 );

   // 检查bool类型字段
   var actRecords7 = dbOpr.findFromCL( dbcl, { "": { "$Text": { query: { "match": { "tobj.test": true } } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expRecords7 = dbOpr.findFromCL( dbcl, { "tobj.test": true }, { _id: { "$include": 0 } }, { _id: 1 } );
   checkResult( expRecords7, actRecords7 );
   
   // 检查date类型字段
   var actRecords8 = dbOpr.findFromCL( dbcl, { "": { "$Text": { query: { "match": { "tobj.test": "2023-05-25" } } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expectRecords8 = dbOpr.findFromCL( dbcl, { "tobj.test": { "$date": "2023-05-25" } }, { _id: { "$include": 0 } }, { _id: 1 } );
   checkResult( expectRecords8, actRecords8 );

   // 检查array类型字段
   var actRecords9 = dbOpr.findFromCL( dbcl, { "": { "$Text": { query: { "match": { "tobj.test": 123 } } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expectRecords9 = dbOpr.findFromCL( dbcl, { "tobj.test": 123 }, { _id: { "$include": 0 } }, { _id: 1 } );
   checkResult( expectRecords9, actRecords9 );

   // 删除索引
   dbcl.dropIndex( idxName );
   checkIndexNotExistInES( esIndexNames );
}
