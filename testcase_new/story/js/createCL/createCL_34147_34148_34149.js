/******************************************************************************
 * @Description   : seqDB-34147:普通表 Group 指定多个数据组
 * @Description   : seqDB-34148:范围分区表 Group 指定多个数据组
 * @Description   : seqDB-34149:主表 Group 指定多个数据组
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
   var groupsArray = commGetGroups( db, false, "", true, true, true );
   var groupNameArray = [];
   for (var i = 0; i < groupsArray.length; i++)
   {
      groupNameArray.push( groupsArray[i][0].GroupName );
   }

   // normal cl with multiple groups
   var clName = 'cl_34147';
   assert.tryThrow( SDB_NO_SHARDINGKEY, function()
   {
      commCreateCL( db, COMMCSNAME, clName, { "Group": groupNameArray } );
   } );

   // range partition cl with multiple groups
   clName = 'cl_34148';
   assert.tryThrow( SDB_NO_SHARDINGKEY, function()
   {
      commCreateCL( db, COMMCSNAME, clName, { "ShardingKey": { "a": 1 }, "ShardingType": "range", "Group": groupNameArray } );
   } );

   // main cl with multiple groups
   clName = 'cl_34149';
   assert.tryThrow( SDB_NO_SHARDINGKEY, function()
   {
      commCreateCL( db, COMMCSNAME, clName, { "IsMainCL": true, "ShardingKey": { "a": 1 }, "ShardingType": "range", "Group": groupNameArray } );
   } );
}
