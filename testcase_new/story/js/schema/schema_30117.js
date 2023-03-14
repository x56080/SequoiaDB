/******************************************************************************
 * @Description   : seqDB-30117:未绑定外部模式不存在数据的集合关闭内部模式
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.02.24
 * @LastEditTime  : 2023.02.24
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_30117";
testConf.clOpt = { EnableInfoSchema: true };
testConf.schemaName = COMMSCHEMANAME + "_30117";
testConf.schemaDef = { "a": { Type: "int32" } };
main( test );

function test ( testPara )
{
   // 重复开启内部模式
   testPara.testCL.alter( { "EnableInfoSchema": true } );

   // 关闭内部模式
   assert.tryThrow( SDB_OPERATION_INCOMPATIBLE, function()
   {
      testPara.testCL.alter( { "EnableInfoSchema": false } );
   } )

   // 开启内部模式并插入数据
   testPara.testCL.alter( { "EnableInfoSchema": true } );
   var doc = [];
   for( var i = 0; i < 20; i++ )
   {
      doc.push( { a: i } );
   }
   testPara.testCL.insert( doc );

   // 执行truncate
   testPara.testCL.truncate();

   // 关闭内部模式
   assert.tryThrow( SDB_OPERATION_INCOMPATIBLE, function()
   {
      testPara.testCL.alter( { "EnableInfoSchema": false } );
   } )

   // 开启内部模式并插入lob
   var filePath = WORKDIR + "/lob30117/";
   var fileName = "filelob_30117";
   var fileSize = 1024;
   deleteTmpFile( filePath );
   var fileMD5 = makeTmpFile( filePath, fileName, fileSize );

   testPara.testCL.alter( { "EnableInfoSchema": true } );
   var lobID = testPara.testCL.putLob( filePath + fileName );

   // 关闭内部模式
   assert.tryThrow( SDB_OPERATION_INCOMPATIBLE, function()
   {
      testPara.testCL.alter( { "EnableInfoSchema": false } );
   } )

   // 绑定外部模式
   testPara.testCL.addSchema( testConf.schemaName );

   testPara.testCL.getLob( lobID, filePath + "checkputlob30117", true );
   var actMD5 = File.md5( filePath + "checkputlob30117" );
   assert.equal( fileMD5, actMD5 );
   deleteTmpFile( filePath );
   deleteTmpFile( filePath + "checkputlob30117" );
}

function deleteTmpFile ( filePath )
{
   try
   {
      File.remove( filePath );
   } catch( e )
   {
      if( e.message != SDB_FNE )
      {
         throw e;
      }
   }
}

function makeTmpFile ( filePath, fileName, fileSize )
{
   if( fileSize == undefined ) { fileSize = 1024 * 100; }
   var fileFullPath = filePath + "/" + fileName;
   File.mkdir( filePath );

   var cmd = new Cmd();
   cmd.run( "dd if=/dev/zero of=" + fileFullPath + " bs=1c count=" + fileSize );
   var md5Arr = cmd.run( "md5sum " + fileFullPath ).split( " " );
   var md5 = md5Arr[0];
   return md5
}