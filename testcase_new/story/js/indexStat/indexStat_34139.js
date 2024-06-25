/******************************************************************************
 * @Description   : seqDB-34139:样本总数小于或等于阈值，查看索引统计信息
 * @Author        : chenzejia
 * @CreateTime    : 2024.06.17
 * @LastEditTime  : 2024.06.17
 * @LastEditors   : chenzejia
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipOneGroup = true;
main( test );

function test ()
{
   var mainCLName = "maincl_34139";
   var subCLNamePrefix = "subcl_34139_";
   var subTableNum = 5;
   var indexName = "idx_34139";
   var totalRecords = 20000;
   var sampleNum = 5000;
   var statmcvlimit = 200000;
   var perSubRecords = totalRecords / subTableNum;

   var mainCL = commCreateCL( db, COMMCSNAME, mainCLName, { "IsMainCL": true, "ShardingKey": { "a": 1 }, "ShardingType": "range" } );
   for( var i = 0; i < subTableNum; i++ )
   {
      var subCLName = subCLNamePrefix + i;
      commCreateCL( db, COMMCSNAME, subCLName, { "ShardingKey": { "a": 1 }, "ShardingType": "hash", AutoSplit: true } );
      mainCL.attachCL( COMMCSNAME + "." + subCLName, { LowBound: { a: perSubRecords * i }, UpBound: { a: perSubRecords * ( i + 1 ) } } );
   }
   var docs = [];

   for( var i = 0; i < totalRecords; i++ )
   {
      docs.push( { a: i, b: i } );
   }
   mainCL.insert( docs );

   commCreateIndex( mainCL, indexName, { "b": 1 } );
   db.analyze( { "Collection": COMMCSNAME + "." + mainCLName, "Index": indexName, "SampleNum": sampleNum } );

   // 设置样本阈值为200000
   db.updateConf( { "statmcvlimit": statmcvlimit } );

   try
   {
      var actIndexStat = mainCL.getIndexStat( indexName, true ).toObj();
      var fracs = actIndexStat.MCV.Frac;
      var values = actIndexStat.MCV.Values;
      delete ( actIndexStat.StatTimestamp );
      delete ( actIndexStat.TotalIndexLevels );
      delete ( actIndexStat.TotalIndexPages );
      delete ( actIndexStat.MCV );

      // 校验MCV字段
      assert.equal( values.length, fracs.length );
      assert.equal( fracs.length, 10000 );
      for( var i = 0; i < fracs.length; i++ )
      {
         assert.equal( 1, fracs[i] );
      }

      // 校验其他字段值 
      var expResult = {
         "Collection": COMMCSNAME + "." + mainCLName,
         "Index": indexName,
         "Unique": false,
         "KeyPattern": { "b": 1 },
         "DistinctValNum": [totalRecords],
         "MinValue": { "b": 0 },
         "MaxValue": { "b": totalRecords - 1 },
         "NullFrac": 0,
         "UndefFrac": 0,
         "SampleRecords": totalRecords,
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

