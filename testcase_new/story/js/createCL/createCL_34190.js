/******************************************************************************
 * @Description   : seqDB-34190:Group 指定非法参数
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
      var clName = 'cl_34190';
      commCreateCL( db, COMMCSNAME, clName, { "Group": [ 1, 2 ] } );
   } );
}
