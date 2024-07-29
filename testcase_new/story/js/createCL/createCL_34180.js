/******************************************************************************
 * @Description   : seqDB-34180:哈希分区表指定 Group 同时指定 SplitGroupStart，取值覆盖 [0, group_num * 2]
 * @Author        : linsuqiang
 * @CreateTime    : 2024.07.17
 * @LastEditTime  : 2024.07.17
 * @LastEditors   : linsuqiang
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipOneGroup = true;
main( test );

function checkOffset( stdCataInfo, currCataInfo, splitGroupStartValue )
{
   assert.equal( currCataInfo.length, stdCataInfo.length );

   for (var i = 0; i < currCataInfo.length; i++)
   {
      var currLow = currCataInfo[i].LowBound[ "" ];
      var stdLow = stdCataInfo[i].LowBound[ "" ];
      assert.equal( currLow, stdLow );

      var currUp = currCataInfo[i].UpBound[ "" ];
      var stdUp = stdCataInfo[i].UpBound[ "" ];
      assert.equal( currUp, stdUp );

      var currGroup = currCataInfo[i].GroupName;
      var espectGroupIdx = (i + splitGroupStartValue) % stdCataInfo.length;
      var espectGroup = stdCataInfo[ espectGroupIdx ].GroupName;
      assert.equal( currGroup, espectGroup );
   }
}

function test ()
{
   // cover all SplitGroupStart values
   var clName = 'cl_34180';
   var currOptions = {
      "ShardingKey": { "a": 1 },
      "ShardingType": "hash",
      "AutoSplit": true,
      "SplitGroupStart": 0
   }
   commCreateCL( db, COMMCSNAME, clName, currOptions );
   var stdResult = db.snapshot( SDB_SNAP_CATALOG, { Name: COMMCSNAME + "." + clName } ).next().toObj();
   var stdCataInfo = stdResult.CataInfo;
   commDropCL( db, COMMCSNAME, clName );

   var splitGroupStartValues = [];
   var groupsArray = commGetGroups( db, false, "", true, true, true );
   for (var i = 1; i < groupsArray.length * 2; i++)
   {
      splitGroupStartValues.push( i );
   }
   for (var i = 0; i < splitGroupStartValues.length; i++)
   {
      var clName = 'cl_34180';
      var currOptions = {
         "ShardingKey": { "a": 1 },
         "ShardingType": "hash",
         "AutoSplit": true,
         "SplitGroupStart": splitGroupStartValues[i]
      };
      commCreateCL( db, COMMCSNAME, clName, currOptions );
      var currResult = db.snapshot( SDB_SNAP_CATALOG, { Name: COMMCSNAME + "." + clName } ).next().toObj();
      var currCataInfo = currResult.CataInfo;
      checkOffset( stdCataInfo, currCataInfo, splitGroupStartValues[i] );
      commDropCL( db, COMMCSNAME, clName );
   }
}
