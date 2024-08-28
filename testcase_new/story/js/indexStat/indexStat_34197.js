/******************************************************************************
 * @Description   : seqDB-34197:存在大量空子表时，查看索引统计信息
 * @Author        : chenzejia
 * @CreateTime    : 2024.08.27
 * @LastEditTime  : 2024.08.27
 * @LastEditors   : chenzejia
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   var mainCLName = "maincl_34197";
   var subCLNamePrefix = "subcl_34197_";
   var indexName = "idx_34197";
   var subTableNum = 7;
   var sampleNum = 200;

   var mainCL = commCreateCL( db, COMMCSNAME, mainCLName, { "IsMainCL": true, "ShardingKey": { "a": 1 } } );

   for( var i = 1; i <= subTableNum; i++ )
   {
      var subCLName = subCLNamePrefix + i;
      commCreateCL( db, COMMCSNAME, subCLName );
      mainCL.attachCL( COMMCSNAME + "." + subCLName, { LowBound: { a: 100 * i }, UpBound: { a: 100 * ( i + 1 ) } } );
   }

   var arr = [];
   for( var i = 0; i < 1000; i++ )
   {
      arr.push( { a: 100 } );
   }
   for( var i = 0; i < 2000; i++ )
   {
      arr.push( { a: 200 } );
   }
   for( var i = 0; i < 2000; i++ )
   {
      arr.push( { a: 300 } );
   }
   mainCL.insert( arr );

   commCreateIndex( mainCL, indexName, { "a": 1 } );
   db.analyze( { "Collection": COMMCSNAME + "." + mainCLName, "Index": indexName, "SampleNum": sampleNum } );

   var actIndexStat = mainCL.getIndexStat( indexName, true ).toObj();
   delete ( actIndexStat.StatTimestamp );
   delete ( actIndexStat.TotalIndexLevels );
   delete ( actIndexStat.TotalIndexPages );

   var expResult = {
      "Collection": COMMCSNAME + "." + mainCLName,
      "Index": indexName,
      "Unique": false,
      "KeyPattern": { "a": 1 },
      "DistinctValNum": [3],
      "MinValue": { "a": 100 },
      "MaxValue": { "a": 300 },
      "NullFrac": 0,
      "UndefFrac": 0,
      "MCV": {
         "Values": [{ a: 100 }, { a: 200 }, { a: 300 }],
         "Frac": [2000, 4000, 4000]
      },
      "SampleRecords": 500,
      "TotalRecords": 5000
   };
   assert.equal( actIndexStat, expResult );

   commDropCL( db, COMMCSNAME, mainCLName );
}

