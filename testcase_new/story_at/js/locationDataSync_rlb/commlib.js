/******************************************************************************
 * @Description   :
 * @Author        : huangyouquan
 * @CreateTime    : 2021.11.24
 * @LastEditTime  : 2022.11.24
 * @LastEditors   : huangyouquan
 ******************************************************************************/
import( "../lib/location_commlib.js" );
import( "../lib/basic_operation/commlib.js" );

function getReplPrimaryName(rg, timeoutSecond ) {
  var doTime = 0 ;
  if ( typeof( timeoutSecond ) != "number" )
  {
      timeoutSecond = 15 ; 
  }
  while( true )
  {
     try {
        var replPrimary = rg.getMaster();
        var replPrimaryName = replPrimary.getHostName() + ":" + replPrimary.getServiceName();
        println( "Group primary is: " + replPrimaryName ) ;
        return replPrimaryName;
     } catch ( e ) {
         if ( SDB_RTN_NO_PRIMARY_FOUND == e && doTime < timeoutSecond )
         {
             sleep( 1000 ) ;
             doTime += 1 ;
         }
         else
         {
             throw e ;
         }
     }
  }
}
