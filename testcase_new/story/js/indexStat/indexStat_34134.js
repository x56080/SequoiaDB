/******************************************************************************
 * @Description   : seqDB-34134:表类型为hash分区表，样本数超过阈值，查看索引统计信息
 * @Author        : chenzejia
 * @CreateTime    : 2024.06.17
 * @LastEditTime  : 2024.06.17
 * @LastEditors   : chenzejia
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   var clName = "cl_34134";
   var indexName = "idx_34134";
   var totalRecords = 20000;
   var sampleNum = 5000;
   var statmcvlimit = 2500;
   var testCL = commCreateCL( db, COMMCSNAME, clName, { ShardingType: "hash", ShardingKey: { a: 1 }, AutoSplit: true } );
   var docs = [];
   var distinctValNumArray = [3, 5, 7, 9, 11];
   for( var i = 0; i < totalRecords; i++ )
   {
      docs.push( { a: i, b: distinctValNumArray[i % distinctValNumArray.length] } );
   }
   testCL.insert( docs );

   commCreateIndex( testCL, indexName, { "b": 1 } );
   db.analyze( { "Collection": COMMCSNAME + "." + clName, "Index": indexName, "SampleNum": sampleNum } );

   // 设置样本阈值为2500
   db.updateConf( { "statmcvlimit": statmcvlimit } );

   try
   {
      var actIndexStat = testCL.getIndexStat( indexName, true ).toObj();
      var values = actIndexStat.MCV.Values;
      var fracs = actIndexStat.MCV.Frac;
      delete ( actIndexStat.StatTimestamp );
      delete ( actIndexStat.TotalIndexLevels );
      delete ( actIndexStat.TotalIndexPages );
      delete ( actIndexStat.MCV );

      //校验数据分布是否与实际一致,假定frac中值上下不超过1000
      var expectValues = [];
      for( var i = 0; i < distinctValNumArray.length; i++ )
      {
         expectValues.push( { b: distinctValNumArray[i] } );
      }
      assert.equal( expectValues, values );
      for( var i = 0; i < values.length; i++ )
      {
         if( fracs[i] < 1000 || fracs[i] > 3000 )
         {
            throw new Error( "the proportion of values in MCV is inconsistent with the actual data distribution,values: " + JSON.stringify( values ) + ", fracs: " + JSON.stringify( fracs ) );
         }
      }

      // 校验其他字段值 
      var expResult = {
         "Collection": COMMCSNAME + "." + clName,
         "Index": indexName,
         "Unique": false,
         "KeyPattern": { "b": 1 },
         "DistinctValNum": [distinctValNumArray.length],
         "MinValue": { "b": distinctValNumArray[0] },
         "MaxValue": { "b": distinctValNumArray[distinctValNumArray.length - 1] },
         "NullFrac": 0,
         "UndefFrac": 0,
         "SampleRecords": statmcvlimit,
         "TotalRecords": totalRecords
      };
      assert.equal( actIndexStat, expResult );
   } finally
   {
      commDropCL( db, COMMCSNAME, clName );
      // 恢复默认配置
      db.deleteConf( { statmcvlimit: 1 } )
   }
}

