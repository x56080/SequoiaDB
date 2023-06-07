/******************************************************************************
 * @Description   : seqDB-31509:创建全文索引，索引字段为TimeStamp类型
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.05.29
 * @LastEditTime  : 2023.05.29
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_es_31509";
testConf.clOpt = { ShardingKey: { _id: 1 }, ShardingType: "hash", AutoSplit: true };

main( test );
function test ( testPara )
{
   var dbcl = testPara.testCL;
   var clName = testConf.clName;

   // 插入数据
   var docs = [{ no: 1, time: { "$timestamp": "2019-09-01-13.14.26.124233" } },
   { no: 2, time: { "$timestamp": "2022-02-22-13.14.26.124233" } }];
   dbcl.insert( docs );

   // 创建全文索引，索引字段为TimeStamp类型
   var idxName = "idx_31509";
   dbcl.createIndex( idxName, { "time": "text" } );
   listIndexCheckNoMappings( dbcl, idxName );

   // 检查全文索引结果
   var dbOpr = new DBOperator();
   var esIndexNames = dbOpr.getESIndexNames( COMMCSNAME, clName, idxName );
   var indexRecordNum = 2;
   checkFullSyncToES( COMMCSNAME, clName, idxName, indexRecordNum, esIndexNames );

   var matchConf1 = { "time": { "lte": "2022-01-01" } };
   var actRecords = dbOpr.findFromCL( dbcl, { "": { "$Text": { query: { "range": matchConf1 } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expectRecords = [{ no: 1, time: { "$timestamp": "2019-09-01-13.14.26.124233" } }];
   checkResult( expectRecords, actRecords );

   // 创建全文索引后再次插入数据
   var newInsertRecords = { no: 3, time: { "$timestamp": "2020-02-02-13.14.26.124233" } };
   dbcl.insert( newInsertRecords );

   // 检查全文索引结果
   indexRecordNum = 3;
   checkFullSyncToES( COMMCSNAME, clName, idxName, indexRecordNum, esIndexNames );
   var matchConf2 = { "time": { "lte": "2020-02-20", "gte": "2020-01-01" } };
   actRecords = dbOpr.findFromCL( dbcl, { "": { "$Text": { query: { "range": matchConf2 } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   checkResult( [newInsertRecords], actRecords );

   // 删除索引
   dbcl.dropIndex( idxName );
   checkIndexNotExistInES( esIndexNames );
}

function listIndexCheckNoMappings ( dbcl, indexName )
{
   var cursor = dbcl.listIndexes();
   while( cursor.next() )
   {
      var actIndexDef = cursor.current().toObj().IndexDef;
      var actIndexName = actIndexDef.name;
      if( actIndexName == indexName )
      {
         if( "Mappings" in actIndexDef )
         {
            throw new Error( "check exist mappings info !, mappings = " + JSON.stringify( actIndexDef.Mappings ) );

         }
      }
   }
   cursor.close();
}