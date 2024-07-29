/******************************************************************************
 * @Description   : seqDB-34154:哈希分区表指定多个重复数据组
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
   var clName = 'cl_34154';
   var groupsArray = commGetGroups( db, false, "", true, true, true );
   var groupNameArray = [];
   for (var i = 0; i < groupsArray.length; i++)
   {
      groupNameArray.push( groupsArray[i][0].GroupName );
   }
   for (var i = 0; i < groupsArray.length - 1; i++)
   {
      groupNameArray.push( groupsArray[i][0].GroupName );
   }
   var cl = commCreateCL( db, COMMCSNAME, clName, { "ShardingKey": { "a": 1 }, "ShardingType": "hash", "Group": groupNameArray } );
   cl.insert([{ a: 1 }, { a: 2 }, { a: 3 }]);
   var result = db.snapshot( SDB_SNAP_CATALOG, { Name: COMMCSNAME + "." + clName } ).next().toObj();
   var cataInfo = result.CataInfo;
   assert.equal( cataInfo.length, groupsArray.length );
   for (var i = 0; i < cataInfo.length; i++)
   {
      var found = false;
      for (var j = 0; j < groupsArray.length; j++)
      {
         if (cataInfo[i].GroupName == groupsArray[j][0].GroupName)
         {
            found = true;
            break;
         }
      }
      assert.equal( found, true );
   }
   commDropCL( db, COMMCSNAME, clName );
}
