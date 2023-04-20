/******************************************************************************
 * @Description   : seqDB-31131:REST接口支持SDB_SNAPSHOT_RECYCLEBIN和SDB_LIST_RECYCLEBIN
 * @Author        : HuangHaimei
 * @CreateTime    : 2023.04.15
 * @LastEditTime  : 2023.04.15
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var csName = "cs_31131";
   var clName = "cl_31131";

   //  清理集合空间
   commDropCS( db, csName );
   cleanRecycleBin( db, csName );

   dbcl = commCreateCL( db, csName, clName );
   db.getCS( csName ).dropCL( clName );
   var type = "Drop";

   tryCatch( ["cmd=snapshot recyclebin"], [0] );
   assert.equal( JSON.parse( infoSplit[1] ).OpType, type, infoSplit );

   tryCatch( ["cmd=list recyclebin"], [0] );
   assert.equal( JSON.parse( infoSplit[1] ).OpType, type, infoSplit );

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
}