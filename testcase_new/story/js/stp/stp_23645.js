/******************************************************************************
 * @Description   : seqDB-23645 stp.getServers()接口验证 
 * @Author        : Siqin Chen
 * @CreateTime    : 2021.03.22
 * @LastEditTime  : 2021.03.22
 * @LastEditors   : Siqin Chen
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var serverGroup = getStpServerNodes();
   var primaryNode = getStpPrimaryNode();
   if (serverGroup.length <= 0)
   {
      throw new Error( "Error: stp.getServers return serverGroup is :" + serverGroup.length );
   } else {
      var primaryInfo = primaryNode["HostName"]+":"+primaryNode["Service"];
      
      var i=0;
      for(i=0; i<serverGroup.length; i++)
      {
         if(serverGroup[i] == primaryInfo)
         {
            break;
         }
      }
      if(i == serverGroup.length)
      {
         throw new Error( "Error: stp.getServers return serverGroup :" + serverGroup +" does not contain primary node : " + primaryInfo);
      }
   }
}
