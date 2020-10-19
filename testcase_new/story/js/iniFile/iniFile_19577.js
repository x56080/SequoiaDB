/************************************
*@Description: seqDB-19577 取消注释不存在的item
*@author:      yinzhen
*@createDate:  2019.10.12
**************************************/
main( test );

function test ()
{
   var filePath = WORKDIR + "/ini19577/";
   var fileName = "file19577";
   var fileFullPath = filePath + fileName;
   makeIniFile( filePath, fileName );

   var section1 = "section1";
   var key1 = "key1";
   var value1 = "value1";
   var fileContent = "[" + section1 + "]\n" +
      key1 + "=" + value1;
   initFile( fileFullPath, fileContent );

   // 取消注释不存在的item 指定section进行设置 
   var iniFile = new IniFile( fileFullPath );
   assert.tryThrow( -211, function()
   {
      iniFile.enableItem( section1, "key2" );
   } );

   // 不指定section进行设置
   assert.tryThrow( -211, function()
   {
      iniFile.enableItem( "key2" );
   } );

   deleteIniFile( filePath );
}
