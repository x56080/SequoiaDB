/******************************************************************************
 * @Description   : seqDB-23711 stp reelect([options])接口验证 
 * @Author        : Siqin Chen
 * @CreateTime    : 2021.03.23
 * @LastEditTime  : 2021.03.23
 * @LastEditors   : Siqin Chen
 ******************************************************************************/
testConf.skipStandAlone = true;
//main( test );

function test()
{
   //1.分别连接stp  server主备节点执行reelect,指定Seconds HostName，覆盖默认和非默认值，HostName覆盖主节点、备节点//超时难以实现自动化
   var primaryNode = getStpPrimaryNode(STPHOSTNAME,STPSVCNAME);
   var primaryStp = new Stp(primaryNode["HostName"], primaryNode["Service"]);
   
   var spareNode = getStpSpareNode(STPHOSTNAME,STPSVCNAME);
   var spareStp = new Stp(spareNode[0]["HostName"], spareNode[0]["Service"]);
   
   primaryStp.reelect({Seconds:60,HostName:spareNode[0]["HostName"]});
   var node = getStpPrimaryNode(STPHOSTNAME,STPSVCNAME);
   var expNode = {HostName:spareNode[0]["HostName"],Service:spareNode[0]["Service"]};
   commCompareObject(node, expNode);
   
   var stp = new Stp(spareNode[0]["HostName"], spareNode[0]["Service"]);
   stp.reelect({HostName:primaryNode["HostName"]});
   node = getStpPrimaryNode(STPHOSTNAME,STPSVCNAME);
   expNode = {HostName:primaryNode["HostName"],Service: primaryNode["Service"]};
   commCompareObject(node, expNode);
   
  //2.分别连接stp client节点执行reelect
   var clientNode = getStpClientNode(STPHOSTNAME,STPSVCNAME);
   var clientStp = new Stp(clientNode["HostName"], clientNode["Service"]);
   clientStp.reelect({HostName:primaryNode["HostName"]});
   node = getStpPrimaryNode(STPHOSTNAME,STPSVCNAME);
   expNode = {HostName:primaryNode["HostName"],Service: primaryNode["Service"]};
   commCompareObject(node, expNode);
   
   //3.client节点，不存在的节点
   try
   {
      var stp = new Stp(STPHOSTNAME,STPSVCNAME);
      stp.reelect({HostName:clientNode["HostName"]});
      throw new Error( "expect fail but success!");
   }
   catch( e )
   {
      if( e != -6 &&  e != -222)
      {
         throw new Error( "stp.reelect client: " + e );
      }
   }
   
   //不存在的节点
   try
   {
      var stp = new Stp(STPHOSTNAME,STPSVCNAME);
      stp.reelect({HostName:"relect23711"});
      throw new Error( "expect fail but success!");
   }
   catch( e )
   {
      if( e != -222 )
      {
         throw new Error( "stp.reelect relect23711: " + e );
      }
   }
   //4.指定非法参数名或者非法参数值执行reelect
   var stp = new Stp(STPHOSTNAME,STPSVCNAME);
   try
   {
      stp.reelect({Seconds:"aa"});
      throw new Error( "expect fail but success!");
   }
   catch( e )
   {
      if( e != -6 )
      {
         throw new Error( "stp.reelect({Seconds:'aa'});: " + e );
      }
   }
   stp.reelect({Seconds1:30});
   primaryNode = getStpPrimaryNode(STPHOSTNAME,STPSVCNAME);
   if (primaryNode == null)
   {
      throw new Error( "stp.reelect({Seconds1:30}) error " );
   }
   //8.reelect过程中做事务转账操作，检查事务操作结果及选主结果 js无法并发操作，手工验证
}