/******************************************************************************
 * @Description   : seqDB-24127:dropCS可以通过参数注入指定其他CS
 * @Author        : Yi Pan
 * @CreateTime    : 2021.04.26
 * @LastEditTime  : 2021.04.26
 * @LastEditors   : Yi Pan
 ******************************************************************************/

main( test );

function test ()
{
   var csname1 = "cs_24127a"
   var csname2 = "cs_24127b"
   var csname3 = "cs_24127c"
   var csname4 = "cs_24127d"
   var clname = "cl_24127";

   //直连coord节点创建删除
   db.createCS( csname1 ).createCL( clname );
   db.createCS( csname2 ).createCL( clname );
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.dropCS( csname1, { "Name": csname2 } );
   } );
   var nodes = commGetCLNodes( db, csname1 + "." + clname );
   commDropCS( db, csname1, true );
   commDropCS( db, csname2, true );

   //直连data节点创建删除
   var data = new Sdb( nodes[0].HostName + ":" + nodes[0].svcname );
   data.createCS( csname3 ).createCL( clname );
   data.createCS( csname4 ).createCL( clname );
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      data.dropCS( csname3, { "Name": csname4 } );
   } );
   commDropCS( data, csname3, true );
   commDropCS( data, csname4, true );
   data.close();
}