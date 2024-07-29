/******************************************************************************
 * @Description   : seqDB-34145:哈希分区表 Group 指定一个或以上的数据组
 * @Description   : seqDB-34146:哈希分区表指定多个 Group 同时指定非 0 的 SplitGroupStart
 * @Author        : linsuqiang
 * @CreateTime    : 2024.07.17
 * @LastEditTime  : 2024.07.17
 * @LastEditors   : linsuqiang
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipOneGroup = true;
main( test );

function test ()
{
   // test string array with 1 group
   var clName = 'cl_34145';
   var groupsArray = commGetGroups( db, false, "", true, true, true );
   var groupName = groupsArray[0][0].GroupName;
   var cl = commCreateCL( db, COMMCSNAME, clName, { "ShardingKey": { "a": 1 }, "ShardingType": "hash", "Group": [ groupName ] } );
   cl.insert([{ a: 1 }, { a: 2 }, { a: 3 }]);
   var result = db.snapshot( SDB_SNAP_CATALOG, { Name: COMMCSNAME + "." + clName } ).next().toObj();
   var cataInfo = result.CataInfo;
   assert.equal( cataInfo.length, 1 );
   assert.equal( cataInfo[0].GroupName, groupName );
   commDropCL( db, COMMCSNAME, clName );

   // test string array with multiple groups (SplitGroupStart = 1)
   clName = 'cl_34146';
   var groupNameArray = [];
   for (var i = 0; i < groupsArray.length; i++)
   {
      groupNameArray.push( groupsArray[i][0].GroupName );
   }
   cl = commCreateCL( db, COMMCSNAME, clName, { "ShardingKey": { "a": 1 }, "ShardingType": "hash", "Group": groupNameArray, "SplitGroupStart": 1 } );
   cl.insert([{ a: 1 }, { a: 2 }, { a: 3 }]);
   result = db.snapshot( SDB_SNAP_CATALOG, { Name: COMMCSNAME + "." + clName } ).next().toObj();
   cataInfo = result.CataInfo;
   assert.equal( cataInfo.length, groupNameArray.length );
   for (var i = 0; i < cataInfo.length; i++)
   {
      assert.equal( cataInfo[i].GroupName == groupNameArray[( i + 1 )%(groupNameArray.length)], true );
   }
   commDropCL( db, COMMCSNAME, clName );
}
