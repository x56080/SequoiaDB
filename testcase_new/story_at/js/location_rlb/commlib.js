import("../lib/basic_operation/commlib.js");
import("../lib/location_commlib.js");

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

function getReplPrimaryNodeID(rg) {
  var nodeID = rg.getDetailObj().toObj().PrimaryNode;
  return nodeID;
}

function checkNodeIsReplPrimary(rg, nodeID, seconds) {
  while (seconds-- > 0) {
    sleep(1000);
    var primary = getReplPrimaryNodeID(rg);
    if (primary === nodeID) {
      break;
    }
  }
  assert.equal(
    nodeID,
    primary,
    "Node[" + nodeID + "] is not replica group primary[" + primary + "]"
  );
}
