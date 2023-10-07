/******************************************************************************
 * @Description   : seqDB-32260:DC.setLocation接口参数校验
 * @Author        : tangtao
 * @CreateTime    : 2023.06.21
 * @LastEditTime  : 2023.06.21
 * @LastEditors   : tangtao
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var location1 = "location_32260_1";
   var arr = new Array( 258 );
   var location2 = arr.join( "a" );

   var hostName = db.getCoordRG().getSlave().getHostName();
   var DC = db.getDC();

   // 1.setLocation接口，参数hostname 为空字符串
   DC.setLocation( "", location1 );

   // 2.setLocation接口，参数hostname 为非字符串类型如整数
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      DC.setLocation( 1, location1 );
   } );

   // 3.setLocation接口，参数hostname 为字符串类型，值为一个有效主机名
   DC.setLocation( hostName, location1 );

   // 4.setLocation接口，参数location 为空字符串
   DC.setLocation( hostName, "" );

   // 5.setLocation接口，参数location 为非字符串类型如整数
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      DC.setLocation( hostName, 1 );
   } );

   // 6.setLocation接口，参数location 为字符串类型，长度超过256个字符
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      DC.setLocation( hostName, location2 );
   } );

   // 7.setLocation接口，参数location 为字符串类型，值为一个有效地址
   DC.setLocation( hostName, location1 );

   // 清理location
   DC.setLocation( hostName, "" );
}
