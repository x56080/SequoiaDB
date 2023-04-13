/******************************************************************************
 * @Description   : seqDB-31038:convLogicalTimeToRealTime接口参数校验
 *                : seqDB-31039:convRealTimeToLogicalTime接口参数校验
 * @Author        : liuli
 * @CreateTime    : 2023.04.07
 * @LastEditTime  : 2023.04.12
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var stp = new Stp( STPHOSTNAME, STPSVCNAME );

   // seqDB-31038: convLogicalTimeToRealTime接口参数校验
   // 合法参数
   var logicalTime = 1679339776883577;
   stp.convLogicalTimeToRealTime( logicalTime );

   // 非法参数
   var logicalTime = "1679339776883577";
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      stp.convLogicalTimeToRealTime( logicalTime );
   } )

   var logicalTime = { $date: "2015-03-13" };
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      stp.convLogicalTimeToRealTime( logicalTime );
   } )

   var logicalTime = { "$timestamp": "2015-06-05-16.10.33.000000" };
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      stp.convLogicalTimeToRealTime( logicalTime );
   } )

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      stp.convLogicalTimeToRealTime();
   } )

   // seqDB-31039:convRealTimeToLogicalTime接口参数校验
   // 合法参数
   var realTime = { "$timestamp": "2015-06-05-16.10.33.000000" };
   stp.convRealTimeToLogicalTime( realTime );

   var realTime = Timestamp( "2015-06-05-16.10.33.000000" );
   stp.convRealTimeToLogicalTime( realTime );

   var realTime = Timestamp( 1433492413, 0 );
   stp.convRealTimeToLogicalTime( realTime );

   // 非法参数
   var realTime = "1679339776883577";
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      stp.convRealTimeToLogicalTime( realTime );
   } )

   var realTime = { "time": "2015-06-05-16.10.33.000000" };
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      stp.convRealTimeToLogicalTime( realTime );
   } )

   var realTime = { $date: "2015-03-13" };
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      stp.convRealTimeToLogicalTime( realTime );
   } )

   var realTime = 1679339776883577;
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      stp.convRealTimeToLogicalTime();
   } )

   stp.close();
}