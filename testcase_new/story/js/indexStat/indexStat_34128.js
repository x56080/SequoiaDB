/******************************************************************************
 * @Description   : seqDB-34128:索引为唯一索引，查看索引统计信息
 * @Author        : chenzejia
 * @CreateTime    : 2024.06.17
 * @LastEditTime  : 2024.06.17
 * @LastEditors   : chenzejia
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   var mainCLName = "maincl_34128";
   var subCLNamePrefix = "subcl_34128_";
   var indexName = "idx_34128";
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
   for( var i = 0; i < totalRecords; i++ )
   {
      docs.push( { a: i, b: distinctNumArray[i % 5] } );
   }
   mainCL.insert( docs );

   commCreateIndex( mainCL, indexName, { "a": 1, "b": 1 }, { unique: true } );
   db.analyze( { "Collection": COMMCSNAME + "." + mainCLName, "Index": indexName, "SampleNum": sampleNum } );

   var actIndexStat = mainCL.getIndexStat( indexName, true ).toObj();
   var mcvFracs = actIndexStat.MCV.Frac;
   var mcvValues = actIndexStat.MCV.Values;
   delete ( actIndexStat.StatTimestamp );
   delete ( actIndexStat.TotalIndexLevels );
   delete ( actIndexStat.TotalIndexPages );
   delete ( actIndexStat.MCV );
   delete ( actIndexStat.MaxValue );

   var dataGroupNames = commGetDataGroupNames( db );
   var sampleRecords = dataGroupNames.length * sampleNum * ( subTableNum - 1 ) + ( ( dataGroupNames.length * sampleNum ) / perSubRecords ) * lastSubRecords;
   assert.equal( mcvValues.length, mcvFracs.length );
   assert.equal( mcvFracs.length, sampleRecords < 10000 ? sampleRecords : 10000 );
   // 校验MCV.Frac字段（frac数组所有元素相等）
   var expect = mcvFracs[0];
   for( var i = 0; i < mcvFracs.length; i++ )
   {
      assert.equal( mcvFracs[i], expect, "the frac is not expected,frac:" + mcvFracs );
   }
   // 校验MCV.Values字段
   var subcl1Sample = 0;
   var subcl2Sample = 0;
   var subcl3Sample = 0;
   var subcl4Sample = 0;
   var subcl5Sample = 0;
   for( var i = 0; i < mcvValues.length; i++ )
   {
      var value = mcvValues[i];
      var a = value.a;
      if( i < dataGroupNames.length * sampleNum )
      {
         if( !( a >= 0 && a < perSubRecords ) )
         {
            throw new Error( "the mcv values is not expected,a:" + a );
         }
         subcl1Sample++;
      } else if( i < dataGroupNames.length * sampleNum * 2 )
      {
         if( !( a >= perSubRecords * 1 && a < perSubRecords * 2 ) )
         {
            throw new Error( "the mcv values is not expected,a:" + a );
         }
         subcl2Sample++;
      } else if( i < dataGroupNames.length * sampleNum * 3 )
      {
         if( !( a >= perSubRecords * 2 && a < perSubRecords * 3 ) )
         {
            throw new Error( "the mcv values is not expected,a:" + a );
         }
         subcl3Sample++;
      } else if( i < dataGroupNames.length * sampleNum * 4 )
      {
         if( !( a >= perSubRecords * 3 && a < perSubRecords * 4 ) )
         {
            throw new Error( "the mcv values is not expected,a:" + a );
         }
         subcl4Sample++;
      } else
      {
         if( !( a >= perSubRecords * 4 ) )
         {
            throw new Error( "the mcv values is not expected,a:" + a );
         }
         subcl5Sample++;
      }
   }
   assert.equal( subcl1Sample, subcl2Sample );
   assert.equal( subcl1Sample, subcl3Sample );
   assert.equal( subcl1Sample, subcl4Sample );
   if( subcl5Sample >= subcl1Sample )
   {
      throw new Error( "the mcv values is not expected,subcl1Sample:" + subcl1Sample + ",subcl2Sample:" + subcl5Sample );
   }

   // 校验总样本数和其他字段
   var expResult = {
      "Collection": COMMCSNAME + "." + mainCLName,
      "Index": indexName,
      "Unique": true,
      "KeyPattern": { "a": 1, "b": 1 },
      "DistinctValNum": [sampleRecords, sampleRecords],
      "MinValue": { "a": 0, "b": 1 },
      "NullFrac": 0,
      "UndefFrac": 0,
      "SampleRecords": sampleRecords,
      "TotalRecords": totalRecords
   };
   assert.equal( actIndexStat, expResult );

   commDropCL( db, COMMCSNAME, mainCLName );
}

