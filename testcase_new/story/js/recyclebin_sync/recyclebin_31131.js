/******************************************************************************
 * @Description   : seqDB-31131:REST接口支持SDB_SNAPSHOT_RECYCLEBIN和SDB_LIST_RECYCLEBIN
 * @Author        : HuangHaimei
 * @CreateTime    : 2023.04.15
 * @LastEditTime  : 2023.05.09
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
   var fullName = csName + "." + clName;

   var filter = { OriginName: fullName };
   tryCatch( ["cmd=snapshot recyclebin", "filter=" + JSON.stringify( filter )], [0] );
   assert.equal( JSON.parse( infoSplit ).OriginName, fullName, infoSplit );

   tryCatch( ["cmd=list recyclebin", "filter=" + JSON.stringify( filter )], [0] );
   assert.equal( JSON.parse( infoSplit ).OriginName, fullName, infoSplit );

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
}