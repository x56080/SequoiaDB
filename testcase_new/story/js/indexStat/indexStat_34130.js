/******************************************************************************
 * @Description   : seqDB-34130:采样数大于总记录数，查看索引统计信息
 * @Author        : chenzejia
 * @CreateTime    : 2024.06.17
 * @LastEditTime  : 2024.06.17
 * @LastEditors   : chenzejia
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   var mainCLName = "maincl_34130";
   var subCLNamePrefix = "subcl_34130_";
   var indexName = "idx_34130";
   var subTableNum = 5;
   var totalRecords = 8200;
   var lastSubRecords = 200;
   var sampleNum = 10000;
   var perSubRecords = ( totalRecords - lastSubRecords ) / ( subTableNum - 1 );

   var mainCL = commCreateCL( db, COMMCSNAME, mainCLName, { "IsMainCL": true, "ShardingKey": { "a": 1 }, "ShardingType": "range" } );
   for( var i = 0; i < subTableNum; i++ )
   {
      var subCLName = subCLNamePrefix + i;
      commCreateCL( db, COMMCSNAME, subCLName, { "ShardingKey": { "a": 1 }, "ShardingType": "hash", AutoSplit: true } );
      mainCL.attachCL( COMMCSNAME + "." + subCLName, { LowBound: { a: perSubRecords * i }, UpBound: { a: perSubRecords * ( i + 1 ) } } );
   }
   var docs = [];
   var distinctNumArray = [1, 3, 5, 7, 9];
   for( var i = 0; i < subTableNum - 1; i++ )
   {
      for( var j = 0; j < perSubRecords; j++ )
      {
         docs.push( { a: j + perSubRecords * i, b: distinctNumArray[i] } );
      }
   }
   for( var i = 0; i < lastSubRecords; i++ )
   {
      docs.push( { a: i + perSubRecords * ( subTableNum - 1 ), b: distinctNumArray[subTableNum - 1] } );
   }
   mainCL.insert( docs );

   commCreateIndex( mainCL, indexName, { "b": 1 } );
   db.analyze( { "Collection": COMMCSNAME + "." + mainCLName, "Index": indexName, "SampleNum": sampleNum } );

   var actIndexStat = mainCL.getIndexStat( indexName, true ).toObj();
   var sampleRecords = actIndexStat.SampleRecords;
   delete ( actIndexStat.StatTimestamp );
   delete ( actIndexStat.TotalIndexLevels );
   delete ( actIndexStat.TotalIndexPages );
   delete ( actIndexStat.SampleRecords );

   // 校验总样本数
   assert.equal( sampleRecords, totalRecords );

   // 校验其他字段
   var values = [];
   for( var i = 0; i < distinctNumArray.length; i++ )
   {
      values.push( { "b": distinctNumArray[i] } );
   }
   var fracs = [2439, 2439, 2439, 2439, 243];
   var sampleRecords = totalRecords;
   var expResult = {
      "Collection": COMMCSNAME + "." + mainCLName,
      "Index": indexName,
      "Unique": false,
      "KeyPattern": { "b": 1 },
      "DistinctValNum": [distinctNumArray.length],
      "MinValue": { "b": distinctNumArray[0] },
      "MaxValue": { "b": distinctNumArray[distinctNumArray.length - 1] },
      "NullFrac": 0,
      "UndefFrac": 0,
      "MCV": {
         "Values": values,
         "Frac": fracs
      },
      "TotalRecords": totalRecords
   };
   assert.equal( actIndexStat, expResult );

   commDropCL( db, COMMCSNAME, mainCLName );
}

