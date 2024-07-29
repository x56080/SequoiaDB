/******************************************************************************
 * @Description   : seqDB-34192:RefFrom 指定非法参数
 * @Author        : linsuqiang
 * @CreateTime    : 2024.07.17
 * @LastEditTime  : 2024.07.17
 * @LastEditors   : linsuqiang
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      var clName = 'cl_34192';
      commCreateCL( db, COMMCSNAME, clName, { "RefFrom": { a: 1 } } );
   } );
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      var clName = 'cl_34192';
      commCreateCL( db, COMMCSNAME, clName, { "RefFrom": 1 } );
   } );
}
