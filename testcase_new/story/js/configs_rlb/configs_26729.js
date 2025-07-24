/******************************************************************************
 * @Description   : seqDB-26729:preferredinstance测试配置动态生效
 * @Author        : HuangHaimei
 * @CreateTime    : 2022.07.19
 * @LastEditTime  : 2022.07.20
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
main( test );
function test ()
{
   try
   {
      //  1. 修改正式名preferredinstance校验
      var config = { "preferredinstance": "A" };
      db.updateConf( config );
      var expectConfig = { "preferredinstance": "A", "preferedinstance": "A", };
      checkSnapshot( db, expectConfig );
      // 删除正式名
      db.deleteConf( config );
      var defaultConfig = { "preferredinstance": "M", "preferedinstance": "M" };
      // 检验正式名和别名是否恢复默认值M
      checkSnapshot( db, defaultConfig );
      //  2. 修改别名preferedinstance校验
      var config = { "preferedinstance": "S" };
      db.updateConf( config );
      var expectConfig = { "preferredinstance": "S", "preferedinstance": "S" };
      checkSnapshot( db, expectConfig );
      // 删除别名
      db.deleteConf( config );
      // 检验正式名，别名是否恢复默认值M
      checkSnapshot( db, defaultConfig );
      // 3.校验正式名和别名同时修改，结果是否以正式名为主
      var config = { "preferredinstance": "A", "preferedinstance": "S" };
      db.updateConf( config );
      var expectConfig = { "preferredinstance": "A", "preferedinstance": "A" };
      checkSnapshot( db, expectConfig );
      // 同时删除删除正式名和别名，检验是否恢复为默认值M
      db.deleteConf( config );
      checkSnapshot( db, defaultConfig );
   } finally
   {
      db.deleteConf( { "preferredinstance": 1, "preferedinstance": 1 } );
   }
}
function checkSnapshot ( sdb, option )
{
   // 使用快照查看配置
   var cursor = sdb.snapshot( SDB_SNAP_CONFIGS );
   while( cursor.next() )
   {
      var obj = cursor.current().toObj();
      for( var key in option )
      {
         assert.equal( obj[key], option[key] );
      }
   }
   //关闭游标
   cursor.close();
}