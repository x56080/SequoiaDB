/******************************************************************************
 * @Description   : seqDB-31519:创建多键全文索引，部分字段设置不建立映射关系
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.06.05
 * @LastEditTime  : 2023.06.05
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_es_31519";

main( test );
function test ( testPara)
{
   dbcl = testPara.testCL;
   clName = testConf.clName;

   // 插入数据
   var docs = [{ no: 1, tstr: "2041-03-04", tdouble: -1.7E+308, tbool: true},
   { no: -2147483648, tstr: "test02", tdouble: 1.7E+308, tbool: false },
   { no: 2147483647, tstr: "test01", tdouble: -0.123, tbool: false },
   { no: 2147483646, tstr: "2020-01-03T02:00:00", tdouble: 234.56, tbool: true },
   { no: -1, tstr: "测试数据类型01", tdouble: 5000.46, tbool: false }];
   dbcl.insert( docs );

   // 创建全文索引
   var idxName = "idx_31519";
   var mappingsInfo = { "Fields": { "tstr": { "Type": "keyword", "Index": false }, "tdouble": { "Type": "double" }  } };
   dbcl.createIndex( idxName, { tstr: "text", tdouble: "text", tbool: "text" }, { "Mappings": mappingsInfo } );
   listIndexCheckMappings( dbcl, idxName, mappingsInfo );

   // 检查全文索引结果
   var dbOpr = new DBOperator();
   var esIndexNames = dbOpr.getESIndexNames( COMMCSNAME, clName, idxName );
   var indexRecordNum = 5;
   checkFullSyncToES( COMMCSNAME, clName, idxName, indexRecordNum, esIndexNames );
   var actRecords1 = dbOpr.findFromCL( dbcl, { "": { "$Text": { query: { "match_all": {} } } } }, { no: { "$include": 0 } }, { no: 1 } );
   var expectRecords1 = dbOpr.findFromCL( dbcl, {}, { no: { "$include": 0 } }, { no: 1 } );
   checkResult( expectRecords1, actRecords1 );

   // 检查string类型字段
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      dbOpr.findFromCL( dbcl, { "": { "$Text": { "query": { "prefix": { "tstr": { "value": "test" } } } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   } );
   
   // 检查double类型字段
   var actRecords3 = dbOpr.findFromCL( dbcl, { "": { "$Text": { "query": { "match": { "tdouble": 5000.46 } } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expectRecords3 = dbOpr.findFromCL( dbcl, { "tdouble": 5000.46 }, { _id: { "$include": 0 } }, { _id: 1 } );
   checkResult( expectRecords3, actRecords3 );

   // 检查boolean类型字段
   var actRecords4 = dbOpr.findFromCL( dbcl, { "": { "$Text": { "query": { "match": { "tbool": true } } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expectRecords4 = dbOpr.findFromCL( dbcl, { "tbool": true }, { _id: { "$include": 0 } }, { _id: 1 } );
   checkResult( expectRecords4, actRecords4 );

   // 删除索引
   dbcl.dropIndex( idxName );
   checkIndexNotExistInES( esIndexNames );
}