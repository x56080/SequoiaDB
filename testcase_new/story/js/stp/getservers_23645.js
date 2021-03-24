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
   testGetServer23645();
}

function testGetServer23645()
{
   var serverGroup = getStpServerNodes(STPHOSTNAME, STPSVCNAME);
   var primaryNode = getStpPrimaryNode(STPHOSTNAME,STPSVCNAME);
   var isSuccess = true
   if (serverGroup.length <= 0)
   {
      isSuccess = false;
   }
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
      isSuccess = false;
   }
   
   if (!isSuccess)
   {
      throw new Error( "Error: stp.getServers return servers is not expected!" );
   }
}
