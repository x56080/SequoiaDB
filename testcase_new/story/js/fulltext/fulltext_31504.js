/******************************************************************************
 * @Description   : seqDB-31504 :: 创建全文索引，索引字段为int类型
 * @Author        : wu yan 
 * @CreateTime    : 2023.05.11
 * @LastEditTime  : 2023.05.11
 * @LastEditors   : wu yan
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_es_31504";
testConf.clOpt = { ShardingKey: { no: 1 }, ShardingType: "hash", AutoSplit: true };

main( test );
function test ( testPara )
{
   var dbcl = testPara.testCL;
   var clName = testConf.clName;
   // 创建全文索引前插入数据

   var recordNum = 10000;
   var dataGenerator = new commDataGenerator();
   var records = dataGenerator.getRecords( recordNum, "int", ['num'] );
   dbcl.insert( records );

   var indexName = "index_31504";
   dbcl.createIndex( indexName, { "num": "text" } );
   listIndexCheckNoMappings( dbcl, indexName );

   // 全文检索，检查结果   
   var esIndexNames = dbOpr.getESIndexNames( COMMCSNAME, clName, indexName );
   checkFullSyncToES( COMMCSNAME, clName, indexName, recordNum, esIndexNames );
   var actRecords = dbOpr.findFromCL( dbcl, { "": { "$Text": { query: { match_all: {} } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expectRecords = dbOpr.findFromCL( dbcl, {}, { _id: { "$include": 0 } }, { _id: 1 } );
   checkResult( expectRecords, actRecords );

   // 创建全文索引后再次插入数据
   var newInsertRecords = [{ num: 2147483647 }, { num: -2147483648 }];
   dbcl.insert( newInsertRecords );

   // 全文检索，检查结果  
   checkFullSyncToES( COMMCSNAME, clName, indexName, recordNum + 2, esIndexNames );
   var matchConf2 = { "num": -2147483648 };
   actRecords = dbOpr.findFromCL( dbcl, { "": { "$Text": { query: { match: matchConf2 } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   checkResult( [matchConf2], actRecords );

   // 删除索引   
   dbcl.dropIndex( indexName );
   checkIndexNotExistInES( esIndexNames );

   //删除新插入记录
   dbcl.remove( newInsertRecords );
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
            throw new Error( "mappings info should not be displayed !, mappings = " + JSON.stringify( actIndexDef.Mappings ) );

         }
      }
   }
   cursor.close();
}



