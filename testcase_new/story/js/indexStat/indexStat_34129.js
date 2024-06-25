/******************************************************************************
 * @Description   : seqDB-34129:索引为复合索引，查看索引统计信息
 * @Author        : chenzejia
 * @CreateTime    : 2024.06.17
 * @LastEditTime  : 2024.06.17
 * @LastEditors   : chenzejia
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   var mainCLName = "maincl_34129";
   var subCLNamePrefix = "subcl_34129_";
   var indexName = "idx_34129";
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
         docs.push( { a: j + perSubRecords * i, b: distinctNumArray[i], c: distinctNumArray[i] } );
      }
   }
   for( var i = 0; i < lastSubRecords; i++ )
   {
      docs.push( { a: i + perSubRecords * ( subTableNum - 1 ), b: distinctNumArray[subTableNum - 1], c: distinctNumArray[subTableNum - 1] } );
   }
   mainCL.insert( docs );

   commCreateIndex( mainCL, indexName, { "b": 1, "c": 1 } );
   db.analyze( { "Collection": COMMCSNAME + "." + mainCLName, "Index": indexName, "SampleNum": sampleNum } );

   var actIndexStat = mainCL.getIndexStat( indexName, true ).toObj();
   delete ( actIndexStat.StatTimestamp );
   delete ( actIndexStat.TotalIndexLevels );
   delete ( actIndexStat.TotalIndexPages );

   var dataGroupNames = commGetDataGroupNames( db );
   var values = [];
   for( var i = 0; i < distinctNumArray.length; i++ )
   {
      values.push( { "b": distinctNumArray[i], "c": distinctNumArray[i] } );
   }
   var fracs = [2439, 2439, 2439, 2439, 243];
   var sampleRecords = dataGroupNames.length * sampleNum * ( subTableNum - 1 ) + ( ( dataGroupNames.length * sampleNum ) / perSubRecords ) * lastSubRecords;
   var expResult = {
      "Collection": COMMCSNAME + "." + mainCLName,
      "Index": indexName,
      "Unique": false,
      "KeyPattern": { "b": 1, "c": 1 },
      "DistinctValNum": [distinctNumArray.length, distinctNumArray.length],
      "MinValue": { "b": distinctNumArray[0], "c": distinctNumArray[0] },
      "MaxValue": { "b": distinctNumArray[distinctNumArray.length - 1], "c": distinctNumArray[distinctNumArray.length - 1] },
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

