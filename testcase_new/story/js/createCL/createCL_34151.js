/******************************************************************************
 * @Description   : seqDB-34151:哈希分区表指定数据组不存在
 * @Author        : linsuqiang
 * @CreateTime    : 2024.07.17
 * @LastEditTime  : 2024.07.17
 * @LastEditors   : linsuqiang
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   // group does not exist
   clName = 'cl_34151';
   assert.tryThrow( SDB_CLS_GRP_NOT_EXIST, function()
   {
      commCreateCL( db, COMMCSNAME, clName, { "ShardingKey": { "a": 1 }, "ShardingType": "hash", "Group": [ 'inexistentGroup' ] } );
   } );
}
