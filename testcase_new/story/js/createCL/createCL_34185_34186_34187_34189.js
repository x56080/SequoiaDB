/******************************************************************************
 * @Description   : seqDB-34185:同时指定 Group 与 RefObj / RefFrom
 * @Description   : seqDB-34186:Group 指定多个组，同时指定 AutoSplit 为 true
 * @Description   : seqDB-34187:同时指定 RefObj 和 RefFrom
 * @Description   : seqDB-34189:指定 RefMode 为 2，同时指定 SplitGroupStart 非 0
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

   var hashCLName = 'hash_34185';
   commCreateCL( db, COMMCSNAME, hashCLName, {
      ShardingKey: { a: 1 },
      ShardingType: "hash",
      AutoSplit: true,
      ReplSize: 2,
      ConsistencyStrategy: 2,
      StrictDataMode: false } );
   var hashCLObj = db.snapshot( SDB_SNAP_CATALOG, { Name: COMMCSNAME + "." + hashCLName } ).next().toObj();

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      var clName = 'cl_34185';
      commCreateCL( db, COMMCSNAME, clName, { "Group": groupNameArray, "RefFrom": COMMCSNAME + "." + hashCLName } );
   } );
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      var clName = 'cl_34185';
      commCreateCL( db, COMMCSNAME, clName, { "Group": groupNameArray, "RefObj": hashCLObj } );
   } );
   assert.tryThrow( SDB_NO_SHARDINGKEY, function()
   {
      var clName = 'cl_34186';
      commCreateCL( db, COMMCSNAME, clName, { "Group": groupNameArray, "AutoSplit": true } );
   } );
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      var clName = 'cl_34187';
      commCreateCL( db, COMMCSNAME, clName, { "RefFrom": COMMCSNAME + "." + hashCLName, "RefObj": hashCLObj } );
   } );
   var clName = 'cl_34189';
   commCreateCL( db, COMMCSNAME, clName, { "RefFrom": COMMCSNAME + "." + hashCLName, "RefMode": 2, "SplitGroupStart": 1 } );
   commDropCL( db, COMMCSNAME, clName );

   commDropCL( db, COMMCSNAME, hashCLName );
}
