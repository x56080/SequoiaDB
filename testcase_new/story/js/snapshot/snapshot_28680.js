/******************************************************************************
 * @Description   : seqDB-28680:检查回收站项目列表信息
 * @Author        : Xu Mingxing
 * @CreateTime    : 2022.11.09
 * @LastEditTime  : 2022.11.14
 * @LastEditors   : Xu Mingxing
 ******************************************************************************/
main( test );

function test ()
{
   var csName = "csName_28680";
   var clName = "clName_28680";
   commDropCS( db, csName );
   commCreateCS( db, csName );
   db.getCS( csName ).createCL( clName );

   // 删除CL
   db.getCS( csName ).dropCL( clName );
   // 查看回收站项目列表
   var cursor = db.getRecycleBin().list( { OriginName: csName + "." + clName } );
   var recycleName = cursor.current().toObj()["RecycleName"];
   // 恢复集合CL
   var returnName = db.getRecycleBin().returnItem( recycleName ).toObj()["ReturnName"];
   assert.equal( returnName, csName + "." + clName );

   // 删除CS 
   db.dropCS( csName );
   // 查看回收站项目列表
   var cursor = db.getRecycleBin().list( { OriginName: csName } );
   var recycleName = cursor.current().toObj()["RecycleName"];
   // 恢复集合空间CS
   var returnName = db.getRecycleBin().returnItem( recycleName ).toObj()["ReturnName"];
   assert.equal( returnName, csName );

   commDropCS( db, csName );
}