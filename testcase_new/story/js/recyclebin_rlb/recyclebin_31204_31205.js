/******************************************************************************
 * @Description   :seqDB-31204:不使用回收站，dropCL删除CL并添加备注
 *                 seqDB-31205:不使用回收站，CL执行truncate并添加备注
 * @Author        : Bi Qin
 * @CreateTime    : 2023.04.21
 * @LastEditTime  : 2023.04.24
 * @LastEditors   : Bi Qin
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   try
   {
      var csName = "cs_31204";
      var clName1 = "cl_31204_1";
      var clName2 = "cl_31204_2";
      var clName3 = "cl_31205_1";
      var clName4 = "cl_31205_2";
      var comment = "dropCL31204_truncate31205";

      commDropCS( db, csName );
      cleanRecycleBin( db, csName );

      var dbcs = commCreateCS( db, csName );
      var dbcl1 = commCreateCL( db, csName, clName1 );
      var dbcl2 = commCreateCL( db, csName, clName2 );
      var dbcl3 = commCreateCL( db, csName, clName3 );
      var dbcl4 = commCreateCL( db, csName, clName4 );

      insertBulkData( dbcl1, 1000 );
      insertBulkData( dbcl2, 2000 );
      insertBulkData( dbcl3, 3000 );
      insertBulkData( dbcl4, 4000 );
      //seqDB-31204
      dbcs.dropCL( clName1, { Comment: comment, SkipRecycleBin: true } );
      var count = db.getRecycleBin().count( { OriginName: csName + "." + clName1 } );
      assert.equal( count, 0 );
      //seqDB-31205
      dbcl3.truncate( { Comment: comment, SkipRecycleBin: true } );
      count = db.getRecycleBin().count( { OriginName: csName + "." + clName3 } );
      assert.equal( count, 0 );

      db.getRecycleBin().disable();
      assert.equal( db.getRecycleBin().getDetail().toObj().Enable, false );
      //seqDB-31204
      dbcs.dropCL( clName2, { Comment: comment } );
      count = db.getRecycleBin().count( { OriginName: csName + "." + clName2 } );
      assert.equal( count, 0 );
      //seqDB-31205
      dbcl4.truncate( { Comment: comment } );
      count = db.getRecycleBin().count( { OriginName: csName + "." + clName4 } );
      assert.equal( count, 0 );

      commDropCS( db, csName );
      cleanRecycleBin( db, csName );
   } finally
   {
      db.getRecycleBin().enable();
      assert.equal( db.getRecycleBin().getDetail().toObj().Enable, true );
   }
}