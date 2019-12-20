/**************************************
 * @Description: seqDB-10922 :: 版本: 1 :: 指定sessionID不存在
 * @author: Zhao xiaoni
 * @Date: 2019-12-20
 *************************************/
import ("../lib/main.js")

testConf.skipStandAlone = true;

main( test );

function test()
{
   var sessionId = 1000000000;
   try
   {
      db.forceSession( sessionId );
      throw "NEED_ERROR";
   } catch( e )
   {
      if( e !== -62 )
      {
         throw new Error( e );   
      }
   }

    //force一个集群中存在的sessionid，但是options不存在
   var sessions = db.list( SDB_LIST_SESSIONS, { Status: { $ne: "Waiting" }, 
                           Type: { $nin: [ "Agent", "ShardAgent", "CoordAgent", "ReplAgent", "HTTPAgent" ] } } );
   var sessionId = sessions.next().toObj().SessionID;
   try
   {
      db.forceSession( sessionId, { HostName: "sdbserver01", svcname: "21810" } );
      throw "NEED_ERROR";
   } catch( e )
   {
      if( e !== -155 )
      {
         throw new Error( e );  
      }
   }
}
