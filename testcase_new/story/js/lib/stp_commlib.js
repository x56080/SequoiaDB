/******************************************************************************
 * @Description   : stp公共方法
 * @Author        : Siqin Chen
 * @CreateTime    : 2021.03.20
 * @LastEditTime  : 2021.03.20
 * @LastEditors   : Siqin Chen
 ******************************************************************************/
import( "../lib/main.js" );
import( "../lib/basic_operation/commlib.js" );

//获取stp主节点
function getStpPrimaryNode()
{
   var stp = new Stp(STPHOSTNAME,STPSVCNAME);
   var serverGroup = stp.getServers();
   
   var serverInfo = JSON.parse( serverGroup.toString() );

   return serverInfo["PrimaryNode"]
}
//stp.getServers()
//获取stp备节点
function getStpSpareNode()
{
   var stp = new Stp(STPHOSTNAME,STPSVCNAME);
   var serverGroup = stp.getServers();
   
   var primaryNode = getStpPrimaryNode();
   var serverInfo = JSON.parse( serverGroup.toString() );
   
   var spareNodes = [];
   for(var i = 0; i < serverInfo["Group"].length; i++) 
   {
      if (serverInfo["Group"][i]["HostName"] != primaryNode["HostName"])
      {
         spareNodes.push(serverInfo["Group"][i]);
      }
   }
   return spareNodes;
}
//获取stp client节点
function getStpClientNode()
{
   var stp = new Stp(STPHOSTNAME,STPSVCNAME);
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

function getStpServerNodes()
{
   var stp = new Stp(STPHOSTNAME,STPSVCNAME);
   var serverGroup = stp.getServers();
   
   var serverInfo = JSON.parse( serverGroup.toString() );
   var servers = [];
   for(var i=0; i<serverInfo["Group"].length; i++)
   {
      servers.push(serverInfo["Group"][i]["HostName"]+":"+serverInfo["Group"][i]["Service"]);
   }
   return servers;
}

