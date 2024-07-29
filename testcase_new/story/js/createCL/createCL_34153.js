/******************************************************************************
 * @Description   : seqDB-34153:哈希分区表指定 0 个数据组
 * @Author        : linsuqiang
 * @CreateTime    : 2024.07.17
 * @LastEditTime  : 2024.07.17
 * @LastEditors   : linsuqiang
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   var clName = 'cl_34153';
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      commCreateCL( db, COMMCSNAME, clName, { "ShardingKey": { "a": 1 }, "ShardingType": "hash", "Group": [] } );
   } );
}
