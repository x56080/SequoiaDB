/*******************************************************************
* @Description : common function for test
*                            
* @author      : Liang XueWang
*                2018-03-07
*******************************************************************/
function getDataGroups( db )
{
   var groups = new Array() ;
   if( commIsStandalone( db ) )
   {
      return groups ; 
   }
   
   try
   {
      var cursor = db.listReplicaGroups() ;
      var tmpInfo ;
      while( tmpInfo = cursor.next() )
      {
         var groupname = tmpInfo.toObj().GroupName ;
         if( groupname == "SYSCoord" || groupname == "SYSCatalogGroup" )
            continue ;
         groups.push( groupname ) ;
      }
      cursor.close() ;
      
   }catch(e)
   {
      throw new Error(e);
   }
   return groups ;
   
}

function parseGroupNodes( db, groupname )
{
   var arr = new Array() ;
   if( commIsStandalone( db ) )
   {
      return arr ;
   }
   try
   {
      var tmpObj = db.getRG( groupname ).getDetail().next().toObj() ;
      var tmpGroupArray = tmpObj["Group"] ;
      for( var j = 0;j < tmpGroupArray.length;++j )
      {
         var tmpNodeObj = tmpGroupArray[j] ;
         var nodename = tmpNodeObj["HostName"] ;
         for( var k = 0;k < tmpNodeObj.Service.length;++k )
         {
            var tmpSvcObj = tmpNodeObj.Service[k] ;
            if( tmpSvcObj["Type"] == 0 )
            {
               nodename = nodename + ":" + tmpSvcObj["Name"] ;
               arr.push( nodename ) ;
               break ;
            }
         }
      }
      
   }catch(e)
   {
      throw new Error(e);
   }
   return arr ;
   
}