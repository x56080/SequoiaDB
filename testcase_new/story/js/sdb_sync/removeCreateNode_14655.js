/*******************************************************************
* @Description : test create node after remove
*                seqDB-14655:删除节点后立即重建节点                 
* @author      : Liang XueWang
*                2018-03-07
*******************************************************************/
main( db ) ;

function main( db )
{
   if( commIsStandalone( db ) )
   {
      println( "Run mode is standalone" ) ;
      return ; 
   }
   
   // get data group
   var groups = getDataGroups( db ) ;
   var groupname = "" ;
   for( var i = 0;i < groups.length;i++ )
   {
      groupname = groups[i] ;
      var nodes = parseGroupNodes( groupname ) ;
      if( nodes.length >= 2 )
      {
         println( "group: " + groupname ) ;
         break ;
      }
   }
   if( groupname === "" )
   {
      println( "no qualified group" ) ;
      return ;
   }
   
   // create and start node in group
   var rg = db.getRG( groupname ) ;
   var host = System.getHostName() ;
   var svc = RSRVPORTBEGIN ;
   var dbpath = RSRVNODEDIR + "data/" + svc ;
   println( "create node: " + host + ":" + svc ) ;
   rg.createNode( host, svc, dbpath ) ;
   var node = rg.getNode( host, svc ) ;
   node.start() ;
   
   // remove node and create
   rg.removeNode( host, svc ) ;
   rg.createNode( host, svc, dbpath ) ;
   
   // remove node in the end
   rg.removeNode( host, svc ) ;
}