/******************************************************************************
 * @Description   : seqDB-34131:主表挂载超多子表，查看索引统计信息
 * @Author        : chenzejia
 * @CreateTime    : 2024.06.17
 * @LastEditTime  : 2024.06.17
 * @LastEditors   : chenzejia
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   var mainCLName = "maincl_34131";
   var subCLNamePrefix = "subcl_34131_";
   var indexName = "idx_34131";
   var subTableNum = 100;
   var totalRecords = 198200;
   var lastSubRecords = 200;
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
   for( var i = 0; i < subTableNum - 1; i++ )
   {
      for( var j = 0; j < perSubRecords; j++ )
      {
         docs.push( { a: j + perSubRecords * i, b: i + 2 } );
      }
   }
   for( var i = 0; i < lastSubRecords; i++ )
   {
      docs.push( { a: i + perSubRecords * ( subTableNum - 1 ), b: subTableNum - 1 + 2 } );
   }
   mainCL.insert( docs );

   commCreateIndex( mainCL, indexName, { "b": 1 } );
   db.analyze( { "Collection": COMMCSNAME + "." + mainCLName, "Index": indexName, "SampleNum": sampleNum } );

   var actIndexStat = mainCL.getIndexStat( indexName, true ).toObj();
   delete ( actIndexStat.StatTimestamp );
   delete ( actIndexStat.TotalIndexLevels );
   delete ( actIndexStat.TotalIndexPages );

   var values = [];
   var fracs = [];
   for( var i = 0; i < subTableNum; i++ )
   {
      values.push( { "b": i + 2 } );
      fracs.push( 100 );
   }
   fracs.pop();
   fracs.push( 10 );
   var dataGroupNames = commGetDataGroupNames( db );
   var sampleRecords = dataGroupNames.length * sampleNum * ( subTableNum - 1 ) + ( ( dataGroupNames.length * sampleNum ) / perSubRecords ) * lastSubRecords;
   var expResult = {
      "Collection": COMMCSNAME + "." + mainCLName,
      "Index": indexName,
      "Unique": false,
      "KeyPattern": { "b": 1 },
      "DistinctValNum": [subTableNum],
      "MinValue": { "b": 2 },
      "MaxValue": { "b": subTableNum - 1 + 2 },
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

