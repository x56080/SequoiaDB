/******************************************************************************
 * @Description   : seqDB-31530:切分表指定条件范围查询，带limit/skip/sort
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.05.29
 * @LastEditTime  : 2023.05.29
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipOneGroup = true;

// main( test ); SEQUOIADBMAINSTREAM-9618 
function test ()
{
   var dataGroupNames = commGetDataGroupNames( db );
   var clName = COMMCLNAME + "_es_31530";
   var clOpt = { "ShardingKey": { "_id": 1 }, "ShardingType": "hash", "Partition": 256, "Group": dataGroupNames[0] };
   var dbcl = commCreateCL( db, COMMCSNAME, clName, clOpt );

   // 创建全文索引
   var idxName = "idx_31530";
   dbcl.createIndex( idxName, { "tstr": "text" } );

   // 切分
   dbcl.split( dataGroupNames[0], dataGroupNames[1], { "Partition": 80 }, { "Partition": 160 } );
   dbcl.split( dataGroupNames[0], dataGroupNames[2], { "Partition": 160 }, { "Partition": 256 } );

   // 插入数据
   var doc = [
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
   dbcl.insert( doc );

   // 检查全文索引结果
   var dbOpr = new DBOperator();
   var esIndexNames = dbOpr.getESIndexNames( COMMCSNAME, clName, idxName );
   var indexRecordNum = 12
   checkFullSyncToES( COMMCSNAME, clName, idxName, indexRecordNum, esIndexNames );
   var actRecords1 = dbOpr.findFromCL( dbcl, { "": { "$Text": { query: { "match_all": {} } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expectRecords1 = dbOpr.findFromCL( dbcl, {}, { _id: { "$include": 0 } }, { _id: 1 } );
   checkResult( expectRecords1, actRecords1 );

   // limit(size)
   var matchConf1 = { "match_phrase": { "tstr": "字符串" } };
   var actRecords2 = dbOpr.findFromCL( dbcl, { "": { "$Text": { query: matchConf1, size: 1 } } }, { _id: { "$include": 0 } }, { _id: 1 }, {}, 1 );
   var expectRecords2 = [{ no: 1, tstr: "string字符串" }];
   checkResult( expectRecords2, actRecords2 );

   // skip(from)
   var actRecords3 = dbOpr.findFromCL( dbcl, { "": { "$Text": { query: matchConf1, from: 3, size: 2 } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expectRecords3 = [{ no: 6, tstr: "string 字符串 1111" }, { no: 110, tstr: "字符串string1111" }];
   checkResult( expectRecords3, actRecords3 );

   // sort(order)
   var actRecords4 = dbOpr.findFromCL( mainCL, { "": { "$Text": { query: matchConf1, sort:[{ "no": { "order": "asc" }}], size: 3 } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expectRecords4 = [{ no: 1, tstr: "string字符串" }, { no: 2, tstr: "string字符串1111" }, { no: 3, tstr: "string" }];
   checkResult( expectRecords4, actRecords4 );

   // 删除索引
   dbcl.dropIndex( idxName );
   checkIndexNotExistInES( esIndexNames );
}