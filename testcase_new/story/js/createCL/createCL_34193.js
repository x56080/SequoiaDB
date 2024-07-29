/******************************************************************************
 * @Description   : seqDB-34193:RefMode 指定非法参数
 * @Author        : linsuqiang
 * @CreateTime    : 2024.07.17
 * @LastEditTime  : 2024.07.17
 * @LastEditors   : linsuqiang
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   var hashCLName = 'hash_34193';
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
      var clName = 'cl_34193';
      commCreateCL( db, COMMCSNAME, clName, { "RefObj": hashCLObj, "RefMode": -1 } );
   } );
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      var clName = 'cl_34193';
      commCreateCL( db, COMMCSNAME, clName, { "RefObj": hashCLObj, "RefMode": 3 } );
   } );
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      var clName = 'cl_34193';
      commCreateCL( db, COMMCSNAME, clName, { "RefObj": hashCLObj, "RefMode": 'a' } );
   } );

   commDropCL( db, COMMCSNAME, hashCLName );
}
