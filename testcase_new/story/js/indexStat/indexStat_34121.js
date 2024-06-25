/******************************************************************************
 * @Description   : seqDB-34121:子表包含普通表和分区表，获取索引统计信息
 * @Author        : chenzejia
 * @CreateTime    : 2024.06.17
 * @LastEditTime  : 2024.06.17
 * @LastEditors   : chenzejia
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   var mainCLName = "maincl_34121";
   var subCLNamePrefix = "subcl_34121_";
   var indexName = "idx_34121";
   var subTableNum = 5;
   var totalRecords = 82000;
   var lastSubRecords = 2000;
   var sampleNum = 200;
   var perSubRecords = ( totalRecords - lastSubRecords ) / ( subTableNum - 1 );

   var mainCL = commCreateCL( db, COMMCSNAME, mainCLName, { "IsMainCL": true, "ShardingKey": { "a": 1 }, "ShardingType": "range" } );
   for( var i = 0; i < subTableNum; i++ )
   {
      var subCLName = subCLNamePrefix + i;
      if( i == 0 || i == 1 )
      {
         commCreateCL( db, COMMCSNAME, subCLName );
      } else
      {
         commCreateCL( db, COMMCSNAME, subCLName, { "ShardingKey": { "a": 1 }, "ShardingType": "hash", AutoSplit: true } );
      }
      mainCL.attachCL( COMMCSNAME + "." + subCLName, { LowBound: { a: perSubRecords * i }, UpBound: { a: perSubRecords * ( i + 1 ) } } );
   }
   var docs = [];
   var distinctNumArray = [1, 3, 5, 7, 9]
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
   var frac = actIndexStat.MCV.Frac;
   delete ( actIndexStat.StatTimestamp );
   delete ( actIndexStat.TotalIndexLevels );
   delete ( actIndexStat.TotalIndexPages );
   delete ( actIndexStat.MCV.Frac );

   // 校验MCV.Frac字段
   assert.equal( frac.length, distinctNumArray.length );
   // 普通子表Frac
   var commFrac1 = frac[0];
   var commFrac2 = frac[1];
   // 分区表Frac
   var hashFrac1 = frac[2];
   var hashFrac2 = frac[3];
   // 最后一个子表frac
   var lastFrac = frac[4];
   assert.equal( commFrac1, commFrac2 );
   assert.equal( hashFrac1, hashFrac2 );
   if( commFrac1 > hashFrac1 )
   {
      throw new Error( "the Frac is not expected,frac:" + frac );
   }
   if( lastFrac >= commFrac1 )
   {
      throw new Error( "the Frac is not expected,frac:" + frac );
   }

   // 校验其他字段
   var dataGroupNames = commGetDataGroupNames( db );
   var values = [];
   for( var i = 0; i < distinctNumArray.length; i++ )
   {
      values.push( { "b": distinctNumArray[i] } );
   }
   var sampleRecords = sampleNum * 2 + dataGroupNames.length * sampleNum * 2 + ( ( dataGroupNames.length * sampleNum ) / perSubRecords ) * lastSubRecords;
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
         "Values": values
      },
      "SampleRecords": sampleRecords,
      "TotalRecords": totalRecords
   };
   assert.equal( actIndexStat, expResult );

   commDropCL( db, COMMCSNAME, mainCLName );
}

