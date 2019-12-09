/************************************
*@Description: seqDB-19929 IniFile类SDB_INIFILE_ESCAPE测试
*@author:      yinzhen
*@createDate:  2019.10.09
**************************************/
try
{
   main();
}
catch( e )
{
   if( e.constructor === Error )
   {
      println( e.stack );
   }
   throw e;
}

function main ()
{
   var filePath = WORKDIR + "/ini19929/";
   var fileName = "file19929";
   var fileFullPath = filePath + fileName;
   makeIniFile( filePath, fileName );

   var section1 = "section1";
   var key1 = "key1";
   var value1 = "value1\\nvalue2"
   var initContent = "[" + section1 + "]\n" +
      key1 + "=" + value1;
   initFile( fileFullPath, initContent );

   // 指定SDB_INIFILE_ESCAPE获取IniFile对象 
   // 获取item值，包含转义符
   var checkFile = new IniFile( fileFullPath, SDB_INIFILE_ESCAPE );
   var newValue1 = "value1\nvalue2";
   var checkValue1 = checkFile.getValue( section1, key1 );
   compareValue( checkValue1, newValue1 );

   deleteIniFile( filePath );
}
