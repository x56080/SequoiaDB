/******************************************************************************
 * @Description   : seqDB-34123:子表数据量差异较大，查看索引统计信息
 * @Author        : chenzejia
 * @CreateTime    : 2024.06.17
 * @LastEditTime  : 2024.06.17
 * @LastEditors   : chenzejia
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   var mainCLName = "maincl_34123";
   var subCLNamePrefix = "subcl_34123_";
   var indexName = "idx_34123";
   var subTableNum = 5;
   var sampleNum = 180;
   var subClRecordsArray = [2000, 1800, 200, 1700, 20000];
   var sortSubClRecordsArray = subClRecordsArray.slice().sort( function( a, b ) { return b - a; } );
   var perSubRecords = 20000;

   var mainCL = commCreateCL( db, COMMCSNAME, mainCLName, { "IsMainCL": true, "ShardingKey": { "a": 1 }, "ShardingType": "range" } );
   for( var i = 0; i < subTableNum; i++ )
   {
      var subCLName = subCLNamePrefix + i;
      commCreateCL( db, COMMCSNAME, subCLName, { "ShardingKey": { "a": 1 }, "ShardingType": "hash", AutoSplit: true } );
      mainCL.attachCL( COMMCSNAME + "." + subCLName, { LowBound: { a: perSubRecords * i }, UpBound: { a: perSubRecords * ( i + 1 ) } } );
   }
   var docs = [];
   var distinctNumArray = [1, 3, 5, 7, 9];
   for( var i = 0; i < subClRecordsArray.length; i++ )
   {
      for( var j = 0; j < subClRecordsArray[i]; j++ )
      {
         docs.push( { a: j + perSubRecords * i, b: distinctNumArray[i] } );
      }
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

   //校验MCV.Frac字段
   assert.equal( frac.length, distinctNumArray.length );
   // 小于等于中位采样率的子表
   var ltMiddleSample1 = frac[subClRecordsArray.indexOf( sortSubClRecordsArray[0] )];
   var ltMiddleSample2 = frac[subClRecordsArray.indexOf( sortSubClRecordsArray[1] )];
   var ltMiddleSample3 = frac[subClRecordsArray.indexOf( sortSubClRecordsArray[2] )];
   // 大于中位采样率的子表
   var gtMiddleSample1 = frac[subClRecordsArray.indexOf( sortSubClRecordsArray[3] )];
   var gtMiddleSample2 = frac[subClRecordsArray.indexOf( sortSubClRecordsArray[4] )];
   assert.equal( ltMiddleSample1, ltMiddleSample2 );
   assert.equal( ltMiddleSample1, ltMiddleSample3 );
   if( gtMiddleSample1 >= ltMiddleSample1 )
   {
      throw new Error( "the Frac is not expected,frac:" + frac );
   }
   if( gtMiddleSample2 >= gtMiddleSample1 )
   {
      throw new Error( "the Frac is not expected,frac:" + frac );
   }

   // 校验其他字段
   var values = [];
   for( var i = 0; i < distinctNumArray.length; i++ )
   {
      values.push( { "b": distinctNumArray[i] } );
   }
   // 此时中位采样率为SampleNum*数据组数/1800
   var dataGroupNames = commGetDataGroupNames( db );
   var sampleRecords = dataGroupNames.length * sampleNum * ( subTableNum - 2 ) + ( ( dataGroupNames.length * sampleNum ) / sortSubClRecordsArray[2] ) * ( sortSubClRecordsArray[subClRecordsArray.length - 1] + sortSubClRecordsArray[subClRecordsArray.length - 2] );
   var totalRecords = 0;
   for( var i = 0; i < subClRecordsArray.length; i++ )
   {
      totalRecords += subClRecordsArray[i];
   }
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

