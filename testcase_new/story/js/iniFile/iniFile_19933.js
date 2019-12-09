/************************************
*@Description: seqDB-19933 IniFile类SDB_INIFILE_COLON测试
*@author:      yinzhen
*@createDate:  2019.10.10
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
   var filePath = WORKDIR + "/ini19933/";
   var fileName = "file19933";
   var fileFullPath = filePath + fileName;
   makeIniFile( filePath, fileName );

   // 指定SDB_INIFILE_COLON获取IniFile对象
   var section1 = "section1";
   var key1 = "key1";
   var key2 = "key2";
   var value1 = "value1";
   var value2 = "value2";
   var iniFile = new IniFile( fileFullPath, SDB_INIFILE_COLON );
   iniFile.setValue( section1, key1, value1 );
   iniFile.setValue( key2, value2 );
   iniFile.save();

   // 指定section进行获取
   var checkFile = new IniFile( fileFullPath, SDB_INIFILE_COLON );
   var checkValue1 = checkFile.getValue( section1, key1 );
   compareValue( checkValue1, value1 );

   // 不指定section进行获取
   var checkValue2 = checkFile.getValue( key2 );
   compareValue( checkValue2, value2 );

   // 检查item分隔符为冒号
   var checkValue1 = checkFile.toString();
   var fileContent = key2 + ":" + value2 + "\n" +
      "[" + section1 + "]\n" +
      key1 + ":" + value1;
   compareValue( fileContent, checkValue1 );

   deleteIniFile( filePath );
}
