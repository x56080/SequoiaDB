/*******************************************************************
* @Description : test connect coord after detach/attach cata nodes
*                seqDB-14521:执行detach/attach编目节点后，连接协调节点                 
* @author      : Liang XueWang
*                2018-02-28
*******************************************************************/
main( db ) ;

function main( db )
{
   if( commIsStandalone( db ) )
   {
      println( "Run mode is standalone." ) ;
      return ;
   }
   
   // at least 2 coord nodes, one to detach and attach cata nodes
   // another one to test connect
   var coordNodes = parseGroupNodes( "SYSCoord" ) ;
   if( coordNodes.length < 2 )
   {
      println( "Coord nodes too few." ) ;
      return ;
   }
   
   // at least 3 cata nodes, 
   // two slave nodes to detach and attach, and reelect after master stop
   var cataNodes = parseGroupNodes( "SYSCatalogGroup" ) ;
   if( cataNodes.length < 3 )
   {
      println( "Cata nodes too few." ) ;
      return ;
   }
   db.close() ;
   
   // connect coord1 and close
   var db1 = new Sdb( coordNodes[0] ) ;
   db1.close() ;
   
   // connect coord2 and detach attach slave cata node
   var db2 = new Sdb( coordNodes[1] ) ;
   var cataRg = db2.getRG( "SYSCatalogGroup" ) ;
   var cataMaster = cataRg.getMaster() ;
   var cataMasterNode = cataMaster.getHostName() + ":" + cataMaster.getServiceName() ;
   println( "Cata master node: " + cataMasterNode ) ;
   detachAndAttachCataNode( cataRg, cataMasterNode, cataNodes ) ;
   
   // stop cataMaster and wait new master reelected
   // wait sync then stop cata master
   waitSync( cataMasterNode, cataNodes ) ;
   cataMaster.stop() ;
   try
   {
      waitNewMaster( cataRg ) ;
   
      // connect coord1 again
      try
      {
         db1 = new Sdb( coordNodes[0] ) ;
         db1.close() ;
      }
      catch( e )
      {
         throw buildException( "main", e, "connect in the end", 0, e ) ;
      }
      //throw "errorrrrrr";
      // start cataMaster
      cataMaster.start() ;
      println("finish....");
   }
   finally
   {
      println("finally....");
      cataRg.start();
      db2.close() ;
   }
}

/******************************************************************
 * detach then attach slave cata node
 * master node can't be detached
 * cataNode: cata nodes array, ex [ "ubuntu-057:11820", .... ]
 ******************************************************************/
function detachAndAttachCataNode( cataRg, cataMasterNode, cataNodes )
{
   for( var i = 0;i < cataNodes.length;i++ )
   {
      if( cataNodes[i] !== cataMasterNode )
      {
         var nodeInfo = cataNodes[i].split( ":" ) ;
         var host = nodeInfo[0] ;
         var svc = nodeInfo[1] ;
         try
         {
            cataRg.detachNode( host, svc, {KeepData : true}) ;
         }
         catch( e )
         {
            throw buildException( "detachAndAttachCataNode", e, "detach node " +
                  cataNodes[i], 0, e ) ;
         }
      }
   }
   
   for( var i = 0;i < cataNodes.length;i++ )
   {
      if( cataNodes[i] !== cataMasterNode )
      {
         var nodeInfo = cataNodes[i].split( ":" ) ;
         var host = nodeInfo[0] ;
         var svc = nodeInfo[1] ;
         while( true )
         {
            try
            {
               cataRg.attachNode( host, svc , {KeepData : true}) ;
               break ;
            }
            catch( e )
            {
               if( e === -10 )  // attach after detach immediately, may cause system error 
                  continue ;
               else 
                  throw buildException( "detachAndAttachCataNode", e, "attach node " +
                        cataNodes[i], 0, e ) ;
            }
         }
      }
   }
}

/******************************************************************
 * wait until cata nodes lsn equal
 * cataNode: cata nodes array, ex [ "ubuntu-057:11820", .... ]
 ******************************************************************/
function waitSync( cataMasterNode, cataNodes )
{
   var cmd = new Cmd() ;
   var interval = 1 ;
   var begin = cmd.run( "date +%s" ).split( "\n" )[0] ;

   var master = new Sdb( cataMasterNode ) ;
   var completeLsn = master.snapshot( SDB_SNAP_DATABASE ).next().
                     toObj().CompleteLSN ;
   master.close() ;
   for( var i = 0;i < cataNodes.length;i++ )
   {
      if( cataNodes[i] === cataMasterNode )
         continue ;
      var slave = new Sdb( cataNodes[i] ) ;
      do
      {
         var lsn = slave.snapshot( SDB_SNAP_DATABASE ).next().
                   toObj().CompleteLSN ;
         cmd.run( "sleep " + interval ) ;
      } while( lsn !== completeLsn ) ;
      slave.close() ;
   }
   println( "CompleteLSN: " + completeLsn ) ;   

   var end = cmd.run( "date +%s" ).split( "\n" )[0] ;
   println( "it takes " + (end-begin) + "s to wait sync" ) ;
}

/******************************************************************
 * wait until master elected
 * cataRg: cata group
 ******************************************************************/
function waitNewMaster( cataRg )
{
   var cmd = new Cmd() ;
   var interval = 1 ; 
   var begin = cmd.run( "date +%s" ).split( "\n" )[0] ;
   while( true )
   {
      try
      {
         cataRg.getMaster() ;
         break ;
      }
      catch( e )
      {
         // println( "getMaster return code: " + e ) ;
         cmd.run( "sleep " + interval ) ;
         continue ;
      }
   }
   var end = cmd.run( "date +%s" ).split( "\n" )[0] ;
   println( "it takes " + (end-begin) + "s to get new master" ) ;
}
