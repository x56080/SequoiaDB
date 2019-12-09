/************************************************************************
*@Description:   seqDB-9114:导入非法MinKey类型数据                                 
*@Author:        2016-7-20  wuyan
************************************************************************/
var clName = COMMCLNAME + "_9114";
main();
function main ()
{
   try
   {
      var cl = readyCL( COMMCSNAME, clName );
      cmd.run( 'rm -rf ./sdbimport.log' );

      //-------------test a：--------------------------
      //import datas          
      var imprtFile = tmpFileDir + "9114.json";
      var srcDatas = "{key:MinKey(123)} "
      var rcInfos = importData( COMMCSNAME, clName, imprtFile, srcDatas );

      //check the Return Infos of the import datas
      var parseFail = 1;
      var importRes = 0;
      checkImportReturn( rcInfos, parseFail, importRes );

      /*//check {date1:SdbDate2015-06-05)} error of sdbimport.log 
      var matchInfos = 'find ./ -name "sdbimport.log" |xargs grep "Syntax Error: JSON key \'key\' missing \',\' or \'}\'"';
      var expLogInfo = 'Syntax Error: JSON key \'key\' missing \',\' or \'}\'';
      checkSdbimportLog(matchInfos,expLogInfo);   
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

