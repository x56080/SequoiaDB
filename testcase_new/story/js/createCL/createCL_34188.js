/******************************************************************************
 * @Description   : seqDB-34188:指定 RefMode 但不指定 RefObj / RefFrom
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
      var clName = 'cl_34188';
      commCreateCL( db, COMMCSNAME, clName, { "RefMode": 1 } );
   } );
}
