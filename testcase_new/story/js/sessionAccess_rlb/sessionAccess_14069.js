/* *****************************************************************************
@description: seqDB-14069:createNode()接口中instanceid参数校验
@author：2020-4-15 zhaoxiaoni 
***************************************************************************** */
main();

function main()
{	  
   if( commIsStandalone( db ) )
   {
      println( "run mode is standalone" );
      return;
   }

   var groupName = "rg_14069";
   var hostName = commGetGroups( db )[0][1].HostName;
   var instanceidList = [ 12, 0, 1, 255, "123"];
   var errInstanceidList = [ 12.234, -1, 256, "0x10"];
   createRGAndNodes( db, groupName, hostName, instanceidList.length, instanceidList );	  
   
   for( var i = 0; i < errInstanceidList.length; i++)
   {
      svcName = parseInt( RSRVPORTBEGIN ) + 10 * ( i + 100 ) ;
      dbpath = RSRVNODEDIR + "data/" + svcName;
      var config = { instanceid: errInstanceidList[i], diaglevel: 5 };
      try
      {
          db.getRG( groupName ).createNode( hostName, svcName, dbpath, config );
          throw new Error( "Create node " + hostName + ":" + svcName + " " + dbpath + " config: " + JSON.stringify(config) + "  need error!" );
      }
      catch( e )
      {
         if( e !== -6 )
         {	
            throw new Error( e );
         }
      }
   }
   db.getRG( groupName ).start();
   
   checkResult( groupName, instanceidList );
   db.removeRG( groupName );
}

function checkResult( groupName, instanceidList )
{
   var groupInfo = db.getRG(groupName).getDetail().current().toObj();
   for(var i = 0; i < instanceidList.length; i++ )
   {
      if( instanceidList[i] != 0 && instanceidList[i] != groupInfo.Group[i].instanceid )
      {
         throw "instanceidList[" + i + "] = " + instanceidList[i] + ", groupInfo.Group[" + i + "].instanceid = " + groupInfo.Group[i].instanceid;
      }
   }
}
