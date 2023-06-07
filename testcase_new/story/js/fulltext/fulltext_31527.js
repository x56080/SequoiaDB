/******************************************************************************
 * @Description   : seqDB-31527:主子表指定条件精确查询，带limit/skip/sort
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.05.26
 * @LastEditTime  : 2023.05.29
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.skipStandAlone = true;

//main( test ); SEQUOIADBMAINSTREAM-9618
function test ()
{
   var mainCLName = COMMCLNAME + "_main_31527";
   var subCLName1 = COMMCLNAME + "_sub_31527_1";
   var subCLName2 = COMMCLNAME + "_sub_31527_2";

   // 创建主子表，并挂载子表
   var mainCL = commCreateCL( db, COMMCSNAME, mainCLName, { IsMainCL: true, ShardingKey: { no: 1 }, ShardingType: 'range' } );
   commCreateCL( db, COMMCSNAME, subCLName1 );
   commCreateCL( db, COMMCSNAME, subCLName2 );
   mainCL.attachCL( COMMCSNAME + "." + subCLName1, { LowBound: { no: 0 }, UpBound: { no: 100 } } );
   mainCL.attachCL( COMMCSNAME + "." + subCLName2, { LowBound: { no: 100 }, UpBound: { no: 200 } } );

   // 创建全文索引
   var idxName = "index_31527";
   mainCL.createIndex( idxName, { "tstr": "text" } );

   // 插入数据
   var docs = [
      { no: 1, tstr: "string字符串" },
      { no: 2, tstr: "string字符串1111" },
      { no: 3, tstr: "string" },
      { no: 4, tstr: "str" },
      { no: 5, tstr: "str字符串" },
      { no: 6, tstr: "string 字符串 1111" },
      { no: 116, tstr: "str字符串1111" },
      { no: 117, tstr: "字符串1111" },
      { no: 118, tstr: "字符串string" },
      { no: 119, tstr: "字符串" },
      { no: 110, tstr: "字符串string1111" },
      { no: 111, tstr: "字符串 string 111" }
   ];
   mainCL.insert( docs );

   // 检查全文索引结果
   var dbOpr = new DBOperator();
   var esIndexNames = dbOpr.getESIndexNames( COMMCSNAME, mainCLName, idxName );
   var indexRecordNum = 12;
   checkMainCLFullSyncToES( COMMCSNAME, mainCLName, idxName, indexRecordNum, esIndexNames );
   var actRecords1 = dbOpr.findFromCL( mainCL, { "": { "$Text": { query: { "match_all": {} } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expectRecords1 = dbOpr.findFromCL( mainCL, {}, { _id: { "$include": 0 } }, { _id: 1 } );
   checkResult( expectRecords1, actRecords1 );

   // limit(size)
   var matchConf1 = { "match_phrase": { "tstr": "字符串" } };
   var actRecords2 = dbOpr.findFromCL( mainCL, { "": { "$Text": { query: matchConf1, size: 1 } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expectRecords2 = [{ no: 1, tstr: "string字符串" }];
   checkResult( expectRecords2, actRecords2 );

   // skip(from)
   var actRecords3 = dbOpr.findFromCL( mainCL, { "": { "$Text": { query: matchConf1, from: 3, size: 2 } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expectRecords3 = [{ no: 6, tstr: "string 字符串 1111" }, { no: 110, tstr: "字符串string1111" }];
   checkResult( expectRecords3, actRecords3 );

   // sort(order)
   var actRecords4 = dbOpr.findFromCL( mainCL, { "": { "$Text": { query: matchConf1, sort:[{ "no": { "order": "asc" }}], size: 3 } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expectRecords4 = [{ no: 1, tstr: "string字符串" }, { no: 2, tstr: "string字符串1111" }, { no: 3, tstr: "string" }];
   checkResult( expectRecords4, actRecords4 );

   // 删除索引
   mainCL.dropIndex( idxName );
   checkIndexNotExistInES( esIndexNames );
}