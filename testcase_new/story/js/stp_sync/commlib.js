/******************************************************************************
 * @Description   : stp公共方法
 * @Author        : Siqin Chen
 * @CreateTime    : 2021.03.20
 * @LastEditTime  : 2021.03.20
 * @LastEditors   : Siqin Chen
 ******************************************************************************/
import( "../lib/main.js" );
import( "../lib/basic_operation/commlib.js" );

var installPath = getInstallDir();

function getInstallDir ()
{
   var localDir = cmd.run( "pwd" ).split( "\n" )[0] + "/";
   var installDir = '';

   try
   {
      cmd.run( 'find ./bin/sdbexprt' ).split( '\n' )[0];
      installDir = localDir;
   }
   catch( e ) 
   {
      installDir = commGetInstallPath() + "/";
   }

   return installDir;
}

function testRunCommand ( command, errno )
{
   if( errno == undefined )
   {
      var rc = cmd.run( command );
      return rc;
   } else
   {
      assert.tryThrow( errno, function()
      {
         cmd.run( command );
      } );
   }
}

//server小于2时跳过用例

//获取stp主节点
function getStpPrimaryNode(stpHost, stpPort)
{
   var stp = new Stp(stpHost, stpPort);
   var serverGroup = stp.getServers();
   
   var serverInfo = JSON.parse( serverGroup.toString() );

   return serverInfo["PrimaryNode"]
}
//stp.getServers()
//获取stp备节点
function getStpSpareNode(stpHost, stpPort)
{
   var stp = new Stp(stpHost, stpPort);
   var serverGroup = stp.getServers();
   
   var primaryNode = getStpPrimaryNode(stpHost, stpPort)
   var serverInfo = JSON.parse( serverGroup.toString() );
   
   var spareNode = [];
   for(var i = 0; i < serverInfo["Group"].length; i++) 
   {
      if (serverInfo["Group"][i]["HostName"] != primaryNode["HostName"])
      {
         spareNode.push(serverInfo["Group"][i]);
      }
   }
   return spareNode;
}
//获取stp client节点
function getStpClientNode(stpHost, stpPort)
{
   var stp = new Stp(stpHost, stpPort);
   var syncClients = stp.getSyncClients();
   var stpClientInfo = JSON.parse( syncClients.toString() );
   
   for(var i = 0; i < stpClientInfo["SyncClients"].length; i++) 
   {
      if(stpClientInfo["SyncClients"][i]["Role"] == "client")
      {
         return stpClientInfo["SyncClients"][i];
      }
   }
    return null;
}
//stp.getServers()

function getStpServerNodes(stpHost, stpPort)
{
   var stp = new Stp(stpHost, stpPort);
   var serverGroup = stp.getServers();
   
   var serverInfo = JSON.parse( serverGroup.toString() );
   var servers = [];
   for(var i=0; i<serverInfo["Group"].length; i++)
   {
      servers.push(serverInfo["Group"][i]["HostName"]+":"+serverInfo["Group"][i]["Service"]);
   }
   return servers;
}

