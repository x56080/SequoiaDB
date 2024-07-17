/******************************************************************************
 * @Description   : seqDB-34142:二次采样后子表样本数为0，获取索引统计信息
 * @Author        : chenzejia
 * @CreateTime    : 2024.07.17
 * @LastEditTime  : 2024.07.17
 * @LastEditors   : chenzejia
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );

function test ()
{
   var mainCLName = "maincl_34142";
   var subCLName1 = "subcl_34142_1";
   var subCLName2 = "subcl_34142_2";
   var subCLName3 = "subcl_34142_3";
   var indexName = "index_34142";
   var statmcvlimit = 20;

   var mainCL = commCreateCL( db, COMMCSNAME, mainCLName, { "IsMainCL": true, "ShardingKey": { "a": 1 }, "ShardingType": "range" } );
   commCreateCL( db, COMMCSNAME, subCLName1 );
   commCreateCL( db, COMMCSNAME, subCLName2 );
   commCreateCL( db, COMMCSNAME, subCLName3 );

   mainCL.attachCL( COMMCSNAME + "." + subCLName1, { LowBound: { a: 100 }, UpBound: { a: 200 } } );
   mainCL.attachCL( COMMCSNAME + "." + subCLName2, { LowBound: { a: 200 }, UpBound: { a: 300 } } );
   mainCL.attachCL( COMMCSNAME + "." + subCLName3, { LowBound: { a: 300 }, UpBound: { a: 400 } } );

   // 子表1插入特别少的数据
   mainCL.insert( { a: 100, b: 1 } );
   for( var i = 0; i < 100; i++ )
   {
      mainCL.insert( { a: 200, b: i + 100 } );
      mainCL.insert( { a: 300, b: i + 200 } );
   }

   commCreateIndex( mainCL, indexName, { "b": 1 } );
   db.analyze( { "Collection": COMMCSNAME + "." + mainCLName, "Index": indexName } );

   // 设置样本阈值为20
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
      assert.equal( fracs.length, 20 );
      for( var i = 0; i < fracs.length; i++ )
      {
         assert.equal( 500, fracs[i] );
      }

      // 校验其他字段值 
      var expResult = {
         "Collection": COMMCSNAME + "." + mainCLName,
         "Index": indexName,
         "Unique": false,
         "KeyPattern": { "b": 1 },
         "DistinctValNum": [statmcvlimit],
         "MinValue": { "b": 1 },
         "MaxValue": { "b": 299 },
         "NullFrac": 0,
         "UndefFrac": 0,
         "SampleRecords": statmcvlimit,
         "TotalRecords": 201
      };
      assert.equal( actIndexStat, expResult );
   } finally
   {
      commDropCL( db, COMMCSNAME, mainCLName );
      // 恢复默认配置
      db.deleteConf( { statmcvlimit: 1 } );
   }
}

