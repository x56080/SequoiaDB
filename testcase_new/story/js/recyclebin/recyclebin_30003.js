/******************************************************************************
 * @Description   : seqDB-30003:删除回收站项目中最后一个CL，同步删除数据节点中相应CS的数据文件
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.02.07
 * @LastEditTime  : 2023.02.07
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var csName = "cs_30003";
   var clName = "cl_30003";

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );

   var dataGroupName = commGetDataGroupNames( db )[0];
   var option = { Group: dataGroupName }
   var dbcl = commCreateCL( db, csName, clName, option );
   var docs = [{ a: 1, b: 1 }, { a: 2, b: 2 }, { a: 3, b: 3 }];
   dbcl.insert( docs );

   // 删除CL
   var dbcs = db.getCS( csName );
   dbcs.dropCL( clName );

   // 删除回收站Cl项目
   var clRecycleName = getOneRecycleName( db, csName + "." + clName, "Drop" );
   db.getRecycleBin().dropItem( clRecycleName );

   // 检查回收站，CL项目已被删除
   checkRecycleItem( clRecycleName );

   // 数据节点查询cs是否已被删除
   assert.tryThrow( SDB_DMS_CS_NOTEXIST, function()
   {
      var data = db.getRG( dataGroupName ).getMaster().connect();
      data.getCS( csName );
      data.close();
   } );

   commDropCS( db, csName, true );
}