/******************************************************************************
 * @Description   : seqDB-34173:RefObj 引用无效对象
 * @Description   : seqDB-34174:RefFrom 引用不存在的表
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
   // ref invalid obj
   clName = 'cl_34173';
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      commCreateCL( db, COMMCSNAME, clName, { RefObj: { ShardingKey: { a: 1 }, ShardingType: "hash", CataInfo: [] } } );
   } );

   // ref inexistent cl
   clName = 'cl_34174';
   assert.tryThrow( SDB_DMS_NOTEXIST, function()
   {
      commCreateCL( db, COMMCSNAME, clName, { RefFrom: "inexistent.thiscl" } );
   } );
}
