/******************************************************************************
 * @Description   : stp公共方法
 * @Author        : Siqin Chen
 * @CreateTime    : 2021.03.20
 * @LastEditTime  : 2021.03.20
 * @LastEditors   : Siqin Chen
 ******************************************************************************/

if( typeof ( STPHOSTNAME ) == "undefined" ) { STPHOSTNAME = 'localhost'; }
//STP服务端端口号，默认传入9622
if( typeof ( STPSVCNAME ) == "undefined" ) { STPSVCNAME = '9622'; }

if( typeof ( FUNCTION ) == "undefined" ) { FUNCTION = 'getStpPrimaryNode'; }

var stp = new Stp(STPHOSTNAME, STPSVCNAME);
var serverInfo = stp.getServers().toObj();
   
main();

function main()
{
   if (FUNCTION == "getStpPrimaryNode")
   {
      getStpPrimaryNode();
   }
   if (FUNCTION == "getStpSpareNode")
   {
      getStpSpareNode();
   }
   if (FUNCTION == "getStpClientNode")
   {
      getStpClientNode();
   }
   if (FUNCTION == "getStpServerNodes")
   {
      getStpServerNodes();
   }
   if (FUNCTION == "getStpTimeUsAndTimeError")
   {
      getStpTimeUsAndTimeError();
   }
}
//server小于2时跳过用例

//获取stp主节点
function getStpPrimaryNode()
{
   println(serverInfo["PrimaryNode"]["HostName"]+":"+serverInfo["PrimaryNode"]["Service"]);
}
//stp.getServers()
//获取stp备节点
function getStpSpareNode()
{
   var spareNode = [];
   for(var i = 0; i < serverInfo["Group"].length; i++) 
   {
      if (serverInfo["Group"][i]["HostName"] != serverInfo["PrimaryNode"]["HostName"])
      {
         spareNode.push(serverInfo["Group"][i]["HostName"]+":"+serverInfo["Group"][i]["Service"]);
      }
   }
   println(spareNode);
}
//获取stp client节点
function getStpClientNode()
{
   var syncClients = stp.getSyncClients();
   var stpClientInfo = JSON.parse( syncClients.toString() );
   var client = "";
   for(var i = 0; i < stpClientInfo["SyncClients"].length; i++) 
   {
      if(stpClientInfo["SyncClients"][i]["Role"] == "client")
      {
         client=stpClientInfo["SyncClients"][i]["HostName"]+":"+stpClientInfo["SyncClients"][i]["Service"];
         println(client);
         return;
      }
   }
    
}
//stp.getServers()

function getStpServerNodes()
{
   var servers = [];
   for(var i=0; i<serverInfo["Group"].length; i++)
   {
      servers.push(serverInfo["Group"][i]["HostName"]+":"+serverInfo["Group"][i]["Service"]);
   }
   return servers;
}

function getStpTimeUsAndTimeError()
{
   var timeUs = JSON.parse( stp.getTimeUS().toString() );
   println(timeUs["TimeStamp"]+","+timeUs["TimeError"]);
}

