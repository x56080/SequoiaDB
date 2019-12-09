/*******************************************************************
* @Description : test create node after remove
*                seqDB-14655:删除节点后立即重建节点                 
* @author      : Liang XueWang
*                2018-03-07
*******************************************************************/
import( "../sdb_sync/commlib.js" );
main( db );

function main ( db )
{
   if( commIsStandalone( db ) )
   {
      println( "Run mode is standalone" );
      return;
   }

   // get data group
   var groups = getDataGroups( db );
   var groupname = "";
   for( var i = 0; i < groups.length; i++ )
   {
      groupname = groups[i];
      var nodes = parseGroupNodes( groupname );
      if( nodes.length >= 2 )
      {
         println( "group: " + groupname );
         break;
      }
   }
   if( groupname === "" )
   {
      println( "no qualified group" );
      return;
   }

   // create and start node in group
   var rg = db.getRG( groupname );
   var host = System.getHostName();
   var svc = parseInt( RSRVPORTBEGIN ) + 10;
   var dbpath = RSRVNODEDIR + "data/" + svc;
   var checkSucc = false;
   var times = 0;
   var maxRetryTimes = 10;
   try
   {
      do
      {
         try
         {
            rg.createNode( host, svc, dbpath );
            println( "create node: " + host + ":" + svc );
            checkSucc = true;
         }
         catch( e )
         {
            //-145 :SDBCM_NODE_EXISTED  -290:SDB_DIR_NOT_EMPTY
            if( e == -145 || e == -290 )
            {
               svc = svc + 10;
               dbpath = RSRVNODEDIR + "data/" + svc;
            }
            else
            {
               throw "create node failed!  svc = " + svc + " dbpath = " + dbpath + " errorCode: " + e;
            }
            times++;
         }
      }
      while( !checkSucc && times < maxRetryTimes );

      var node = rg.getNode( host, svc );
      node.start();

      // remove node and create
      rg.removeNode( host, svc );
      rg.createNode( host, svc, dbpath );
   }
   catch( e )
   {
      throw "catch exception : " + e;
   }
   finally
   {
      // remove node in the end
      try
      {
         rg.removeNode( host, svc );
      }
      catch( e )
      {
         // -155:Node does not exist
         if( e !== -155 )
         {
            throw e;
         }
      }
   }
}