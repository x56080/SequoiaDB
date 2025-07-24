/******************************************************************************
 * @Description   :seqDB-26842:创建AutoIndexId为false的表，truncate后获取索引信息
 * @Author        : HuangHaimei
 * @CreateTime    : 2022.08.24
<<<<<<< HEAD
 * @LastEditTime  : 2022.08.25
=======
 * @LastEditTime  : 2022.09.05
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var csName = "cs_26842";
   var clName = "cl_26842";

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );

   var dbcs = commCreateCS( db, csName );
   var dbcl = dbcs.createCL( clName, { AutoIndexId: false } );

   dbcl.truncate();
   var expResult = [];
   var cursor1 = dbcl.snapshotIndexes();
   commCompareResults( cursor1, expResult );
   var recycleName = getOneRecycleName( db, csName + "." + clName, "Truncate" );
   db.getRecycleBin().returnItem( recycleName );
   var cursor2 = dbcl.snapshotIndexes();
   commCompareResults( cursor2, expResult );

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
   checkRecycleItem( recycleName );
}