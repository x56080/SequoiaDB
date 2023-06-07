/******************************************************************************
 * @Description   : seqDB-31507:创建全文索引，索引字段为Bool类型
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.05.29
 * @LastEditTime  : 2023.05.29
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_es_31507";
testConf.clOpt = { ShardingKey: { _id: 1 }, ShardingType: "hash", AutoSplit: true };

main( test );
function test ( testPara )
{
   var dbcl = testPara.testCL;
   var clName = testConf.clName;

   // 插入数据
   var docs = [{ no: 1, a: true }, { no: 2, a: false }];
   dbcl.insert( docs );

   // 创建全文索引，索引字段为Bool类型
   var idxName = "idx_31507";
   dbcl.createIndex( idxName, { "a": "text" } );
   listIndexCheckNoMappings( dbcl, idxName );

   // 检查全文索引结果
   var dbOpr = new DBOperator();
   var esIndexNames = dbOpr.getESIndexNames( COMMCSNAME, clName, idxName );
   var indexRecordNum = 2;
   checkFullSyncToES( COMMCSNAME, clName, idxName, indexRecordNum, esIndexNames );

   var matchConf1 = { "a": true };
   var actRecords = dbOpr.findFromCL( dbcl, { "": { "$Text": { query: { match: matchConf1 } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expectRecords = [{ no: 1, a: true }];
   checkResult( expectRecords, actRecords );

   // 创建全文索引后再次插入数据
   var newInsertRecords = { no: 3, a: false };
   dbcl.insert( newInsertRecords );

   // 检查全文索引结果
   indexRecordNum = 3;
   checkFullSyncToES( COMMCSNAME, clName, idxName, indexRecordNum, esIndexNames );
   var matchConf2 = { "a": false };
   actRecords = dbOpr.findFromCL( dbcl, { "": { "$Text": { query: { match: matchConf2 } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   expectRecords = [{ no: 2, a: false }, { no: 3, a: false }];
   checkResult( expectRecords, actRecords );

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