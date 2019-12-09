/******************************************************************************
 * @Description : test delete run config 
 *                seqDB-14836:删除run级别配置，且配置参数不存在conf文件中
 * @author      : Lu weikang
 * @date        ：2018.3.30
 ******************************************************************************/
import( "../configs/commlib.js" );
main();

function main ()
{
   var hostName = getLocalHostName();
   var svcName = RSRVPORTBEGIN;
   var dbPath = commGetInstallPath() + "/database/data/" + svcName;
   var rg = null;
   var oma = null;
   var svcNameAndsrcLogPath = null;
   var srcLogPath = "";

   try
   {
      //create node 
      if( commIsStandalone( db ) )
      {
         println( "create Node" );
         oma = new Oma( hostName, 11790 );
         oma.createData( svcName, dbPath );
         oma.startNode( svcName );
      }
      else
      {
         var groups = getDataGroups( db );
         var rgName = groups[0];
         rg = db.getRG( rgName );
         svcNameAndsrcLogPath = createAndStartNode( rg, hostName, svcName, dbPath );
         svcName = svcNameAndsrcLogPath["svcName"].toString();
         srcLogPath = svcNameAndsrcLogPath["srcLogPath"];
      }

      //delete conf
      deleteRunConf( db, hostName, svcName );
   }
   catch( e )
   {
      println( "catch e : " + e );
      //将新建组日志备份到/tmp/ci/rsrvnodelog目录下
      var backupDir = "/tmp/ci/rsrvnodelog/14836";
      File.mkdir( backupDir );
      File.scp( srcLogPath, backupDir + "/sdbdiag.log" );
      throw e;
   }
   finally
   {
      // remove node in the end
      if( commIsStandalone( db ) )
      {
         println( "remove Node" );
         oma.removeData( svcName );
      }
      else
      {
         removeNode( rg, hostName, svcName );
      }
   }
}

function deleteRunConf ( db, hostName, svcName )
{
   // before update, get conf file info,get conf snapshot info
   var fileInfo = getConfFromFile( hostName, svcName );
   var snapshotInfo = getConfFromSnapshot( hostName, svcName );

   var runConf = getRandomRunConf();
   var confName = runConf.name;
   var confDefVal = runConf.defVal;
   var config = { confName: 1 };
   var options = { HostName: hostName, svcname: svcName };

   //delete config not in the conf file
   println( "delete conf: " + confName );
   db.deleteConf( config, options );

   // check delete with snapshot and file
   var conf = { confName: confDefVal };
   var snapshotInfo1 = getConfFromSnapshot( hostName, svcName );
   checkSnapshot( snapshotInfo, snapshotInfo1, conf );
   var fileInfo1 = getConfFromFile( hostName, svcName );
   checkConfFile( fileInfo, fileInfo1, conf );

}









