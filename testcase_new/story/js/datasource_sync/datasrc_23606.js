/******************************************************************************
 * @Description   : seqDB-23606:创建数据源指定本地集群
 * @Author        : liuli
 * @CreateTime    : 2021.03.09
 * @LastEditTime  : 2021.03.11
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
// CI环境不支持运行数据源串行用例，暂时屏蔽
// main( test );

function test ()
{
   var dataSrcName = "datasrc23606";
   var coordUrl = getCoordUrl( db );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.createDataSource( dataSrcName, COORDHOSTNAME + ":" + COORDSVCNAME );
   } );
   var obj = getLastErrObj();
   assert.equal( obj.toObj().detail, "'localhost' is not allowed in data source address" );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.createDataSource( dataSrcName, coordUrl[0] );
   } );
   var obj = getLastErrObj();
   assert.equal( obj.toObj().detail, "Data source can not point to local cluster" );

   datasrcDB.close();
}
