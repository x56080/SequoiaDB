/******************************************************************************
 * @Description   : seqDB-32227:putLob接口插入数据2
 * @Author        : tangtao
 * @CreateTime    : 2023.06.15
 * @LastEditTime  : 2023.06.15
 * @LastEditors   : tangtao
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.csName = COMMCSNAME + "_32227";
testConf.clName = COMMCLNAME + "_32227";

main(test);

function test( agrs )
{
   var cl = agrs.testCL;

   var dataFile = "put_lob_32227_data.txt";
   var readFile = "put_lob_32227_read.txt";

   var data = "put_lob_test";
   generateLobFile( dataFile, data );


   // case 1: no param
   assert.tryThrow( SDB_OUT_OF_BOUND, function ()
   {
      res = cl._putLobFile();
   } );

   // case 2: filePath is ""
   assert.tryThrow( SDB_FNE, function () {
      res = cl._putLobFile( "" );
   });

   // case 3: filePath is error type
   assert.tryThrow( SDB_INVALIDARG, function ()
   {
      res = cl._putLobFile( 123 );
   } );

   // case 4: normal filePath
   putAndcheckLob( cl, undefined, dataFile, readFile );

   // case 5: oid is error type
   assert.tryThrow( SDB_INVALIDARG, function ()
   {
      res = cl._putLobFile( dataFile, 123 );
   } );

   // case 6:  normal filePath and oid
   putAndcheckLob( cl, undefined, dataFile, readFile );

   // case 7: lagre file
   data = generateLobData( 256 * 1024 );
   generateLobFile( dataFile, data );
   assert.tryThrow( SDB_INVALIDSIZE, function ()
   {
      res = cl._putLobFile( dataFile );
   } );

   removeLobFile( dataFile );
}