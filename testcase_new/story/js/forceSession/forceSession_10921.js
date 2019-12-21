/**************************************
 * @Description: seqDB-10921 :: 版本: 1 :: 指定sessionID为系统EDU
 * @author: Zhao xiaoni 
 * @Date: 2019-12-20
 **************************************/
testConf.skipStandAlone = true;

main( test );

function test()
{
   // 获取所有系统EDU类型的session，并随机从中取得一个用于force
   var sessions = db.list( 2, { Global: true, Status: { $ne: "Waiting" },
                                Type: { $nin: [ "Agent", "ShardAgent", "CoordAgent", "ReplAgent", "HTTPAgent" ] } } );
   var sessionID = sessions.next().toObj().SessionID;

   //list加条件是因为通过sessionid有可能也能查到非系统EDU类型的session
   var expResult = db.list( 2, { Global: true, SessionID: sessionID, Status: { $ne: "Waiting" }, 
                                 Type: { $nin: [ "Agent", "ShardAgent", "CoordAgent", "ReplAgent", "HTTPAgent" ] } } ).toArray();

   try 
   {
      db.forceSession( sessionID, { Global: true } );
   }
   catch( e ) 
   {
      if( e != -264 )
      {
         throw new Error( e );
      }
   }

   var actResult = db.list( 2, { Global: true, SessionID: sessionID, Status: { $ne: "Waiting" }, 
                                 Type: { $nin: [ "Agent", "ShardAgent", "CoordAgent", "ReplAgent", "HTTPAgent" ] } } ).toArray();
   if( expResult.length !== actResult.length )
   {
      throw new Error( "The expected count is " + expResult.length + ", but the actual count is " + actResult.length );
   }
}
