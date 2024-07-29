/******************************************************************************
 * @Description   : seqDB-34177:哈希分区表指定多个 Group 同时指定 SplitGroupStart 为 -1
 * @Description   : seqDB-34178:RefObj / RefFrom 引用哈希分区表同时指定 SplitGroupStart 为 -1
 * @Description   : seqDB-34179:哈希分区指定 AutoSplit 同时指定 SplitGroupStart 为 -1
 * @Author        : linsuqiang
 * @CreateTime    : 2024.07.17
 * @LastEditTime  : 2024.07.17
 * @LastEditors   : linsuqiang
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipOneGroup = true;
main( test );

function checkGroupSameBound( cataInfo1, cataInfo2 )
{
   assert.equal( cataInfo1.length, cataInfo2.length );
   for (var i = 0; i < cataInfo1.length; i++)
   {
      var low1 = cataInfo1[i].LowBound[ "" ];
      var up1 = cataInfo1[i].UpBound[ "" ];
      var found = false;
      for (var j = 0; j < cataInfo2.length; j++)
      {
         var low2 = cataInfo2[j].LowBound[ "" ];
         var up2 = cataInfo2[j].UpBound[ "" ];
         if (low1 == low2 && up1 == up2)
         {
            found = true;
            break;
         }
      }
      assert.equal( found, true );
   }
}

function checkStartGroupRandom( db, csName, clName, options )
{
   commCreateCL( db, csName, clName, options );
   var firstResult = db.snapshot( SDB_SNAP_CATALOG, { Name: COMMCSNAME + "." + clName } ).next().toObj();
   var firstCataInfo = firstResult.CataInfo;
   commDropCL( db, csName, clName );

   const retry_times = 32;
   var hasDiffStart = false;
   for (var i = 0; i < retry_times; i++)
   {
      commCreateCL( db, csName, clName, options );
      var currResult = db.snapshot( SDB_SNAP_CATALOG, { Name: COMMCSNAME + "." + clName } ).next().toObj();
      var currCataInfo = currResult.CataInfo;
      if (firstCataInfo[0].GroupName != currCataInfo[0].GroupName)
      {
         hasDiffStart = true;
         checkGroupSameBound( firstCataInfo, currCataInfo );
         commDropCL( db, csName, clName );
         break;
      }
      commDropCL( db, csName, clName );
   }
   assert.equal(hasDiffStart, true);
}

function test ()
{
   // Group
   var clName = 'cl_34177';
   var groupsArray = commGetGroups( db, false, "", true, true, true );
   var groupNameArray = [];
   for (var i = 0; i < groupsArray.length; i++)
   {
      groupNameArray.push( groupsArray[i][0].GroupName );
   }
   var groupOptions = {
      "ShardingKey": { "a": 1 },
      "ShardingType": "hash",
      "Group": groupNameArray,
      "SplitGroupStart": -1
   };
   checkStartGroupRandom( db, COMMCSNAME, clName, groupOptions );

   // RefFrom / RefObj
   var refCLName = 'ref_34178';
   commCreateCL( db, COMMCSNAME, refCLName, groupOptions );
   var clName = 'cl_34178';
   var refOptions = {
      "RefFrom": COMMCSNAME + "." + refCLName,
      "SplitGroupStart": -1
   }
   checkStartGroupRandom( db, COMMCSNAME, clName, refOptions );
   commDropCL( db, COMMCSNAME, refCLName );

   // AutoSplit
   var clName = 'cl_34179';
   var autoOptions = {
      "ShardingKey": { "a": 1 },
      "ShardingType": "hash",
      "AutoSplit": true,
      "SplitGroupStart": -1
   }
   checkStartGroupRandom( db, COMMCSNAME, clName, autoOptions );
}
