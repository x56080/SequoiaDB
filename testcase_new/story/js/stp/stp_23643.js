/******************************************************************************
 * @Description   : seqDB-23643 stp.getConf()接口验证 
 * @Author        : Siqin Chen
 * @CreateTime    : 2021.03.22
 * @LastEditTime  : 2021.03.22
 * @LastEditors   : Siqin Chen
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   //1.分别在server/client上执行stp.getConf()获取当前stp节点配置信息
   var primaryNode = getStpPrimaryNode();
   var clientNode = getStpClientNode();
   var clientHost = clientNode["HostName"];
   var clientPort = clientNode["Service"];
   
   var primaryStp = new Stp(primaryNode["HostName"], primaryNode["Service"]);
   var clientStp = new Stp(clientHost, clientPort);
   
   var primaryConf = primaryStp.getConf();
   var conf = JSON.parse( primaryConf.toString());
   checkeConfInfo(conf, "server");
   
   var clientConf = clientStp.getConf();
   conf = JSON.parse( clientConf.toString());
   checkeConfInfo(conf, "client");

   //2.分别在server/client上修改stp节点所有配置项（port除外）该测试点在updaconf接口中验证
}

function checkeConfInfo(stpConf, roleType)
{
   var isSuccess = true
   if (stpConf["role"] != roleType)
   {
      isSuccess = false;
   }
   var serverGroup = getStpServerNodes();
   serverGroup.sort();
   var serverList = stpConf["serverlist"].split(",");
   serverList.sort();
   if(serverGroup.length != serverList.length)
   {
      isSuccess = false;
   }
   for(var i=0; i<serverGroup.length; i++)
   {
      if(serverGroup[i] != serverList[i])
      {
         isSuccess = false;
      }
   }
   if ( !isSuccess )
   {
      throw new Error( "Error: stp.getConf in " + roleType + " return conf is not expected: " + stpConf );
   }
}