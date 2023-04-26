/******************************************************************************
 * @Description   :seqDB-31203:不使用回收站，删除CS并添加备注
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
      var csName1 = "cs_31203_1";
      var csName2 = "cs_31203_2";
      var comment = "dropcs31203";

      commDropCS( db, csName1 );
      commDropCS( db, csName2 );
      cleanRecycleBin( db, "cs_31203_" );

      commCreateCS( db, csName1 );
      commCreateCS( db, csName2 );

      db.dropCS( csName1, { Comment: comment, SkipRecycleBin: true } );

      var count = db.getRecycleBin().count( { OriginName: csName1 } );
      assert.equal( count, 0 );

      db.getRecycleBin().disable();
      assert.equal( db.getRecycleBin().getDetail().toObj().Enable, false );

      db.dropCS( csName2, { Comment: comment } );

      count = db.getRecycleBin().count( { OriginName: csName2 } );
      assert.equal( count, 0 );

      commDropCS( db, csName1 );
      commDropCS( db, csName2 );
      cleanRecycleBin( db, "cs_31203_" );
   } finally
   {
      db.getRecycleBin().enable();
      assert.equal( db.getRecycleBin().getDetail().toObj().Enable, true );
   }
}