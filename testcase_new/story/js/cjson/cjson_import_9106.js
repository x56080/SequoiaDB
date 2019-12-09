/************************************************************************
*@Description:   seqDB-9106:导入非法timestamp类型数据:
                                 a、函数值不满足要求，如{time:Timestamp(12344*aa)}、{time:Timestamp(1433492413)}、{time:Timestamp("2015-06-05-00-16.10.33.000000")} 
                                 b、函数值格式不正确，如{time:Timestamp2015-06-05-00-16.10.33.000000)}、{time:Timestamp('1433492413', 0)} 
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

      //-------------test a：{time:Timestamp(12344*aa)}、{time:Timestamp(1433492413)}、{time:Timestamp("2015-06-05-00-16.10.33.000000")}--------------------------
      //import datas          
      var imprtFile1 = tmpFileDir + "9106a.json";
      var srcDatas1 = "{time:Timestamp(12344*aa)}\n{time:Timestamp(1433492413)}\n{time:Timestamp('2015-16-05-16.10.33.000000')}"
      var rcInfos1 = importData( COMMCSNAME, clName, imprtFile1, srcDatas1 );

      //check the Return Infos of the import datas
      var parseFail1 = 3;
      var importRes1 = 0;
      checkImportReturn( rcInfos1, parseFail1, importRes1 );

      /* //check {time:Timestamp(12344*aa)} error of sdbimport.log 
       var matchInfos1 = 'find ./ -name "sdbimport.log" |xargs grep "Syntax error: unexpected semicolon or newline, expecting \')\'"';
       var expLogInfo1 = 'Syntax error: unexpected semicolon or newline, expecting \')\'';
       checkSdbimportLog(matchInfos1,expLogInfo1);   
       
       //check {time:Timestamp(1433492413)} error of sdbimport.log 
       var matchInfos2 = 'find ./ -name "sdbimport.log" |xargs grep "Failed to read Timestamp, the No.1 argument must be string type"';
       var expLogInfo2 = 'Failed to read Timestamp, the No.1 argument must be string type';
       checkSdbimportLog(matchInfos2,expLogInfo2);
       
       //check sdbimport.log 
       var matchInfos3 = 'find ./ -name "sdbimport.log" |xargs grep "Timestamp month not greater than 12"';
       var expLogInfo3 = 'Timestamp month not greater than 12';
       checkSdbimportLog(matchInfos3,expLogInfo3);
       */
      //---------------------test b：{time:Timestamp2015-06-05-00-16.10.33.000000)}、{time:Timestamp('1433492413', 0)}  -------------------------
      //import datas          
      var imprtFile2 = tmpFileDir + "9106b.json";
      var srcDatas2 = "{time:Timestamp2015-06-05-00-16.10.33.000000)}\n{time:Timestamp('1433492413', 0)}"
      var rcInfos2 = importData( COMMCSNAME, clName, imprtFile2, srcDatas2 );

      //check the Return Infos of the import datas
      var parseFail2 = 2;
      var importRes2 = 0;
      checkImportReturn( rcInfos2, parseFail2, importRes2 );

      //check {time:Timestamp2015-06-05-00-16.10.33.000000)} error of sdbimport.log
      /* var matchInfos4 = 'find ./ -name "sdbimport.log" |xargs grep "ReferenceError: \'Timestamp2015-06-05-00-16.10.33.000000)\' is not defined"';
       var expLogInfo4 = 'ReferenceError: \'Timestamp2015-06-05-00-16.10.33.000000)\' is not defined';
       checkSdbimportLog(matchInfos4,expLogInfo4);     
           
       //check {time:Timestamp(1433492413)} error of sdbimport.log
       var matchInfos5 = 'find ./ -name "sdbimport.log" |xargs grep  "Failed to read Timestamp, the No.1 argument must be integer type"';
       var expLogInfo5 = 'Failed to read Timestamp, the No.1 argument must be integer type';
       checkSdbimportLog(matchInfos5,expLogInfo5);
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

