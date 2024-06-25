/******************************************************************************
 * @Description   : seqDB-34127:索引为分区键，查看索引统计信息
 * @Author        : chenzejia
 * @CreateTime    : 2024.06.17
 * @LastEditTime  : 2024.06.17
 * @LastEditors   : chenzejia
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   var mainCLName = "maincl_34127";
   var subCLNamePrefix = "subcl_34127_";
   var subTableNum = 5;
   var totalRecords = 82000;
   var lastSubRecords = 2000;
   var sampleNum = 200;
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
         docs.push( { a: distinctNumArray[i] + perSubRecords * i, b: distinctNumArray[i] } );
      }
   }
   for( var i = 0; i < lastSubRecords; i++ )
   {
      docs.push( { a: distinctNumArray[subTableNum - 1] + perSubRecords * ( subTableNum - 1 ), b: distinctNumArray[subTableNum - 1] } );
   }
   mainCL.insert( docs );

   db.analyze( { "Collection": COMMCSNAME + "." + mainCLName, "SampleNum": sampleNum } );

   var actIndexStat = mainCL.getIndexStat( "$shard", true ).toObj();
   delete ( actIndexStat.StatTimestamp );
   delete ( actIndexStat.TotalIndexLevels );
   delete ( actIndexStat.TotalIndexPages );

   var values = [];
   for( var i = 0; i < distinctNumArray.length; i++ )
   {
      values.push( { "a": distinctNumArray[i] + perSubRecords * i } );
   }
   var fracs = [2439, 2439, 2439, 2439, 243];
   var sampleRecords = sampleNum * ( subTableNum - 1 ) + ( sampleNum / perSubRecords ) * lastSubRecords;
   var expResult = {
      "Collection": COMMCSNAME + "." + mainCLName,
      "Index": "$shard",
      "Unique": false,
      "KeyPattern": { "a": 1 },
      "DistinctValNum": [distinctNumArray.length],
      "MinValue": { "a": distinctNumArray[0] },
      "MaxValue": { "a": distinctNumArray[distinctNumArray.length - 1] + perSubRecords * 4 },
      "NullFrac": 0,
      "UndefFrac": 0,
      "MCV": {
         "Values": values,
         "Frac": fracs
      },
      "SampleRecords": sampleRecords,
      "TotalRecords": totalRecords
   };
   assert.equal( actIndexStat, expResult );

   commDropCL( db, COMMCSNAME, mainCLName );
}

