/******************************************************************************
 * @Description   : seqDB-23602:创建集合，配置RepairCheck属性
 * @Author        : XiaoNi Huang
 * @CreateTime    : 2021.03.05
 * @LastEditTime  : 2021.03.08
 * @LastEditors   : XiaoNi Huang
 ******************************************************************************/

main( test );
function test ()
{
   var clName = CHANGEDPREFIX + "_cl_23602";
   var cs = commCreateCS( db, COMMCSNAME, true );
   assert.tryThrow( -6, function() { cs.createCL( clName, { "RepairCheck": true } ) } );
}