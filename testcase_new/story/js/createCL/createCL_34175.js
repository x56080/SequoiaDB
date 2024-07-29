/******************************************************************************
 * @Description   : seqDB-34175:使用 RefObj / RefFrom 选项同时指定与源表不同的分区选项
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
   var normalCLName = 'normal_34175';
   var cl = commCreateCL( db, COMMCSNAME, normalCLName, {
      AutoIncrement: { Field: "id" },
      Compressed: false,
      ConsistencyStrategy: 2,
      AutoIndexId: false,
      StrictDataMode: false } );

   var clName = 'cl_34175';
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      commCreateCL( db, COMMCSNAME, clName, { RefFrom: COMMCSNAME + "." + normalCLName, ShardingKey: { a: 1 }, ShardingType: "hash" } );
   } );

   commDropCL( db, COMMCSNAME, normalCLName );
}
