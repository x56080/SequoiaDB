/******************************************************************************
 * @Description   : seqDB-34195:独立模式 RefObj / RefFrom 引用表
 * @Author        : linsuqiang
 * @CreateTime    : 2024.07.17
 * @LastEditTime  : 2024.07.17
 * @LastEditors   : linsuqiang
 ******************************************************************************/
main( test );

function test ()
{
   if ( !commIsStandalone( db ) )
   {
      return;
   }

   var refClName = 'ref_34195';
   commCreateCL( db, COMMCSNAME, refClName, { Compressed: false } );

   var clName = 'cl_34195';
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      commCreateCL( db, COMMCSNAME, clName, { "RefFrom": COMMCSNAME + "." + refClName } );
   } );
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      commCreateCL( db, COMMCSNAME, clName, { "RefObj": { Compressed: false } } );
   } );

   commDropCL( db, COMMCSNAME, refClName );
}
