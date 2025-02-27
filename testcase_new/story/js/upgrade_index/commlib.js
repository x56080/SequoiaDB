/*******************************************************************************
@Description : Upgrade index common functions
@Modify list :
               2025-02-18  fangjiabin  Init
*******************************************************************************/
import( "../lib/basic_operation/commlib.js" ) ;
import( "../lib/main.js" ) ;

function clearCataIndexMeta( clName, idxName, clearAll )
{
   var cata = commGetGroups( db, true, "SYSCatalogGroup", false ) ;
   for( var i = 1 ; i < cata[0].length ; i++ )
   {
      var cataHostName = cata[0][i].HostName ;
      var cataSvcName = cata[0][i].svcname ;
      var catadb = new Sdb( cataHostName, cataSvcName ) ;
      catadb.SYSCAT.SYSINDEXES.remove( { "Collection": clName, "Name": idxName } ) ;
      catadb.close() ;
      if ( !clearAll )
      {
         break ;
      }
   }
}