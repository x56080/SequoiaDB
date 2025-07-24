/******************************************************************************
 * @Description   : seqDB-23640 new stp()获取stp对象
                    seqDB-23641 stp.getTime()接口验证
                    seqDB-23642 stp.getTimeUS()接口验证 
 * @Author        : Siqin Chen
 * @CreateTime    : 2021.03.20
 * @LastEditTime  : 2021.03.20
 * @LastEditors   : Siqin Chen
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var primaryNode = getStpPrimaryNode();
   var spareNode = getStpSpareNode();
   var spareNodeHost = spareNode[0]["HostName"];
   var spareNodePort = spareNode[0]["Service"];
   var clientNode = getStpClientNode();
   var clientHost = clientNode["HostName"];
   var clientPort = clientNode["Service"];
   testConnect23640();
   testTime23641(primaryNode, spareNodeHost, spareNodePort, clientHost, clientPort);
   testTimeUs23642(primaryNode, spareNodeHost, spareNodePort, clientHost, clientPort);
}

function testConnect23640()
{
   //1.分别在server/client执行进入sdb shell，执行new stp() 
   var stp1 = new Stp();
   //2.指定非默认的stp对象，例如：new Stp(hostname/ip,9633) 
   var stp2 = new Stp(STPHOSTNAME,STPSVCNAME);
   //3.指定不存在的stp对象
   try
   {
      var stp3 = new Stp("ccc",9622);
      throw new Error( "expected throw error -15, but success!" );
   }
   catch( e )
   {
      if( e != -15 )
      {
         throw new Error( "new Stp('ccc',9622)" + e );
      }
      
   }
}

function testTime23641(primaryNode, spareNodeHost, spareNodePort, clientHost, clientPort)
{
   //1.分别在server/client主节点上指定stp.getTime()，获取节点的逻辑时间及TimeError 
   
   var primaryStp = new Stp(primaryNode["HostName"], primaryNode["Service"]);
   var spareStp = new Stp(spareNodeHost, spareNodePort);
   var primaryTime1 = primaryStp.getTime();
   var spareTime = spareStp.getTime();
   var primaryTime2 = primaryStp.getTime();
   checkTime(primaryTime1, spareTime, primaryTime2);
   //3.分别在server/client备节点上指定stp.getTime()，获取节点的逻辑时间及TimeError 
   //通过查主再查备再查主，验证逻辑时间正确性 
   var clientStp = new Stp(clientHost, clientPort);
   var clientTime = clientStp.getTime();
   var primaryTime3 = primaryStp.getTime();
   checkTime(primaryTime2, clientTime, primaryTime3);
   
}

function testTimeUs23642(primaryNode, spareNodeHost, spareNodePort, clientHost, clientPort)
{
   //1.分别在server/client主节点上指定stp.getTimeUS()，获取节点的逻辑时间及TimeError 
   var primaryStp = new Stp(primaryNode["HostName"], primaryNode["Service"]);
   var spareStp = new Stp(spareNodeHost, spareNodePort);
   var primaryTime1 = primaryStp.getTimeUS();
   var spareTime = spareStp.getTimeUS();
   var primaryTime2 = primaryStp.getTimeUS();

   var maxTimeError1 = Math.max(primaryTime1["TimeError"], spareTime["TimeError"]);
   var maxTimeError2 = Math.max(spareTime["TimeError"], primaryTime2["TimeError"]);
   var time1 = primaryTime1["TimeStamp"] - maxTimeError1;
   var time2 = spareTime["TimeStamp"];
   var time3 = primaryTime2 + maxTimeError2;

   if ( time2 < time1 || time2 > time3 )
   {
      throw new Error("Dose not meet 'time1 <=  time2 <= time3', time1: "+time1+",time2: "+time2+", time3: "+time3);
   }
   //3.分别在server/client备节点上指定stp.getTimeUS()，获取节点的逻辑时间及TimeError 
   //通过查主再查备再查主，验证逻辑时间正确性 
   var clientStp = new Stp(clientHost, clientPort);
   var clientTime = clientStp.getTimeUS();
   var primaryTime3 = primaryStp.getTimeUS();

   maxTimeError1 = Math.max(primaryTime2["TimeError"], clientTime["TimeError"]);
   maxTimeError2 = Math.max(clientTime["TimeError"], primaryTime3["TimeError"]);
   time1 = primaryTime2["TimeStamp"] - maxTimeError1;
   time2 = clientTime["TimeStamp"];
   time3 = primaryTime3 + maxTimeError2;

   if ( time2 < time1 || time2 > time3 )
   {
      throw new Error("Dose not meet 'time1 <=  time2 <= time3', time1: "+time1+",time2: "+time2+", time3: "+time3);
   }
}

function checkTime(time1, time2, time3)
{
   var time1Info = JSON.parse( time1.toString() );
   var time2Info = JSON.parse( time2.toString() );
   var time3Info = JSON.parse( time3.toString() );
   
   var maxTimeError1 = Math.max(time1Info["TimeError"], time2Info["TimeError"]);
   var maxTimeError2 = Math.max(time2Info["TimeError"], time3Info["TimeError"]);
   
   var time1 = time1Info["TimeStamp"]["Second"]*1000000000 + time1Info["TimeStamp"]["NanoSecond"] - maxTimeError1;
   var time2 = time2Info["TimeStamp"]["Second"]*1000000000 + time2Info["TimeStamp"]["NanoSecond"];
   var time3 = time3Info["TimeStamp"]["Second"]*1000000000 + time3Info["TimeStamp"]["NanoSecond"] + maxTimeError2;
   
   if(time2 < time1 || time2 > time3)
   {
      throw new Error("Dose not meet 'time1 <=  time2 <= time3', time1: "+time1+",time2: "+time2+", time3: "+time3);
   }
}
