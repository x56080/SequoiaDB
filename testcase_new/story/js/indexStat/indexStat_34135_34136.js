/******************************************************************************
 * @Description   : seqDB-34135:表类型为复合分区表，样本数超过阈值，查看索引统计信息
 *                  seqDB-34136:存在大量重复值，样本数超过阈值，查看索引统计信息
 * @Author        : chenzejia
 * @CreateTime    : 2024.06.17
 * @LastEditTime  : 2024.06.17
 * @LastEditors   : chenzejia
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   var mainCLName = "maincl_34135";
   var subCLNamePrefix = "subcl_34135_";
   var subTableNum = 5;
   var indexName = "idx_34135";
   var totalRecords = 20000;
   var sampleNum = 5000;
   var statmcvlimit = 2500;
   var perSubRecords = totalRecords / subTableNum;

   var mainCL = commCreateCL( db, COMMCSNAME, mainCLName, { "IsMainCL": true, "ShardingKey": { "a": 1 }, "ShardingType": "range" } );
   for( var i = 0; i < subTableNum; i++ )
   {
      var subCLName = subCLNamePrefix + i;
      commCreateCL( db, COMMCSNAME, subCLName, { "ShardingKey": { "a": 1 }, "ShardingType": "hash", AutoSplit: true } );
      mainCL.attachCL( COMMCSNAME + "." + subCLName, { LowBound: { a: perSubRecords * i }, UpBound: { a: perSubRecords * ( i + 1 ) } } );
   }
   var docs = [];
   var distinctValNumArray = [3, 5, 7, 9, 11];
   for( var i = 0; i < totalRecords; i++ )
   {
      docs.push( { a: i, b: distinctValNumArray[i % distinctValNumArray.length] } );
   }
   mainCL.insert( docs );

   commCreateIndex( mainCL, indexName, { "b": 1 } );
   db.analyze( { "Collection": COMMCSNAME + "." + mainCLName, "Index": indexName, "SampleNum": sampleNum } );

   // 设置样本阈值为2500
   db.updateConf( { "statmcvlimit": statmcvlimit } );

   try
   {
      var actIndexStat = mainCL.getIndexStat( indexName, true ).toObj();
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
         "Collection": COMMCSNAME + "." + mainCLName,
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
      commDropCL( db, COMMCSNAME, mainCLName );
      // 恢复默认配置
      db.deleteConf( { statmcvlimit: 1 } );
   }
}

