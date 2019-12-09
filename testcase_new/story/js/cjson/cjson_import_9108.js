/************************************************************************
*@Description:   seqDB-9108:导入非法date类型数据:
                                 a、函数值不满足要求，如 {date1:SdbDate2015-06-05)}
                                 b、函数值格式不正确，如{date2:SdbDate("1433492413"}
*@Author:        2016-7-20  wuyan
************************************************************************/
var clName = COMMCLNAME + "_9106";
main();
function main ()
{
   try
   {
      var cl = readyCL( COMMCSNAME, clName );
      cmd.run( 'rm -rf ./sdbimport.log' );

      //-------------test a：--------------------------
      //import datas          
      var imprtFile = tmpFileDir + "9108.json";
      var srcDatas = "{date1:SdbDate2015-06-05)}\n{date2:SdbDate('1433492413'}"
      var rcInfos = importData( COMMCSNAME, clName, imprtFile, srcDatas );

      //check the Return Infos of the import datas
      var parseFail = 2;
      var importRes = 0;
      checkImportReturn( rcInfos, parseFail, importRes );

      /* //check {date1:SdbDate2015-06-05)} error of sdbimport.log 
       var matchInfos1 = 'find ./ -name "sdbimport.log" |xargs grep "ReferenceError: \'SdbDate2015-06-05)\' is not defined"';
       var expLogInfo1 = 'ReferenceError: \'SdbDate2015-06-05)\' is not defined';
       checkSdbimportLog(matchInfos1,expLogInfo1);   
       
       //check {date2:SdbDate('1433492413'} error of sdbimport.log 
       var matchInfos2 = 'find ./ -name "sdbimport.log" |xargs grep "Syntax error: unexpected semicolon or newline, expecting \')\'"';
       var expLogInfo2 = 'Syntax error: unexpected semicolon or newline, expecting \')\'';
       checkSdbimportLog(matchInfos2,expLogInfo2);            
       */
      cleanCL( COMMCSNAME, clName );
      cmd.run( 'rm -rf *.rec' );
      removeTmpDir();
   }
   catch( e )
   {
      throw e;
   }
}

