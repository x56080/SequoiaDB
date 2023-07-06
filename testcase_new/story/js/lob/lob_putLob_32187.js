/******************************************************************************
 * @Description   : seqDB-32187:putLob接口插入数据
 * @Author        : tangtao
 * @CreateTime    : 2023.06.15
 * @LastEditTime  : 2023.06.15
 * @LastEditors   : tangtao
 ******************************************************************************/

testConf.skipStandAlone = true;
testConf.csName = COMMCSNAME + "_32187";
testConf.clName = COMMCLNAME + "_32187";

main(test);

function test( agrs )
{
   var cl = agrs.testCL;

   var data = "put_lob_test";

   // case 1: no param
   assert.tryThrow( SDB_OUT_OF_BOUND, function ()
   {
      res = cl._putLobValue();
   } );

   // case 2: data is ""
   putLobTest( cl, undefined, "" );

   // case 3: data is error type
   assert.tryThrow( SDB_INVALIDARG, function ()
   {
      res = cl._putLobValue( 123 );
   } );

   // case 4: oid is undefined
   putLobTest( cl, undefined, data );

   // case 5: oid is error type
   assert.tryThrow( SDB_INVALIDARG, function ()
   {
      res = cl._putLobValue( data, 123 );
   } );

   // case 6: oid is error type
   assert.tryThrow( SDB_INVALIDARG, function ()
   {
      var lobId = new ObjectId();
      res = cl._putLobValue( data, lobId );
   } );

   // case 7: normal data and oid
   var lobId = cl.createLobID();
   putLobTest( cl, lobId, data );

   // case 8: repeat oid
   assert.tryThrow( SDB_FE, function ()
   {
      res = putLobTest( cl, lobId, data );
   } );

   // case 9: large data
   data = generateLobData( 256 * 1024, 'a' ) ;
   putLobTest( cl, undefined, data );
}

function putLobTest( cl, lobId, data )
{
   if ( undefined == lobId )
   {
      lobId = cl._putLobValue( data );
   }
   else
   {
      cl._putLobValue( data, lobId );
   }

   var dataFile = "put_lob_32187_data.txt";
   var readFile = "put_lob_32187_read.txt";

   generateLobFile( dataFile, data );

   cl.getLob( lobId, readFile, true );

   var expectMD5 = File.md5( dataFile );
   var actualMD5 = File.md5( readFile );

   removeLobFile( dataFile );
   removeLobFile( readFile );

   assert.equal( expectMD5, actualMD5 );
}