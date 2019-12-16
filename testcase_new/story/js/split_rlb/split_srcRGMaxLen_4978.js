/************************************
*@Description: 源分区组名取最大长度值进行数据切分_ST.split.01.001
*@author:      wangkexin
*@createDate:  2019.5.30
*@testlinkCase: seqDB-4978
**************************************/
main();
function main ()
{
   if( true == commIsStandalone( db ) )
   {
      println( "run mode is standalone" );
      return;
   }

   var groupsArray = commGetGroups( db, false, "", true, true, true );

   var csName = COMMCSNAME;
   var clName = CHANGEDPREFIX + "_cl_4978";
   var srcRGName = "";
   var tarRGName = groupsArray[0][0].GroupName;
   var hostName = groupsArray[0][1].HostName;
   var dataNum = 10000;
   for( var i = 0; i < 123; i++ )
   {
      srcRGName += 's';
   }
   srcRGName += "4978";

   //clean environment before test
   commDropCL( db, csName, clName, true, true, "drop CL in the beginning." );
   removeDataRG( srcRGName );


   try
   {
      println( "begin to create data group" );
      var srcLogPath = createDataGroup( srcRGName, hostName );

      var optionObj = { ShardingKey: { a: 1 }, ShardingType: "range", ReplSize: 0, Group: srcRGName };
      var cl = commCreateCL( db, csName, clName, optionObj, true, false );

      //insert data and split
      insertData( cl, dataNum );
      var condition = 0;
      var endcondition = 3000;

      cl.split( srcRGName, tarRGName, { a: condition }, { a: endcondition } );
      println( "begin to check" );
      checkSplitResultByCount( cl, csName, clName, srcRGName, tarRGName, dataNum, 7000, 3000 );
   }
   catch( e )
   {
      println( "catch e : " + e );
      //将新建组日志备份到/tmp/ci/rsrvnodelog目录下
      var backupDir = "/tmp/ci/rsrvnodelog/4978";
      File.mkdir( backupDir );
      File.scp( srcLogPath, backupDir + "/sdbdiag.log" );
      throw e;
   }
   finally
   {
      //清理环境
      commDropCL( db, csName, clName, true, true, "drop CL in the end." );
      removeDataRG( srcRGName );
   }
}