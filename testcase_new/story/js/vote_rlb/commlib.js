function getGroupsWithNodeNum( nodesNum )
{
   var groups = [];
   var commGroups = commGetGroups( db );
   for( var i = 0; i < commGroups.length; i++ )
   {
      var group = commGroups[i];
      if( group.length >= (nodesNum + 1) )
      {
         groups.push( group );
      }
   }
   return groups;
}

function getPrimaryNode( groupName )
{
   try
   {
      var cursor = db.getRG( groupName ).getDetail();
      var primaryNode = cursor.current().toObj().PrimaryNode;
      return primaryNode;
   }
   catch( e )
   {
      throw new Error( e );
   }
}

function notExistPrimaryNode( groupName )
{
   var timeOut = 600;
   var doTimes = 0;
   var period = 0;
   while( doTimes < timeOut )
   {
      var primaryNode = getPrimaryNode( groupName );
      if( primaryNode !== undefined )
      {
         sleep(1000);
         doTimes++;
      } 
      else
      { 
         if( period > 3 )
         {
            break;
         }
         else
         {
            sleep(3000);
            period++;
         }  
      }
   }
   if( doTimes >= timeOut )
   {
      throw new Error( groupName + " has primary node " + primaryNode );
   }
} 

function existPrimaryNode( groupName )
{
   var timeOut = 600;
   var doTimes = 0; 
   while( doTimes < timeOut )
   {  
      var primaryNode = getPrimaryNode( groupName );
      if( primaryNode === undefined )
      {  
         sleep(1000);
         doTimes++;
      } 
      else
      {
         sleep( 3000 );  
         break;
      }
   }

   if( doTimes >= timeOut )
   {  
      throw new Error( groupName + " does't have primary node" );
   }
   else
   {  
      return primaryNode;
   }
}

function getMajorityNodeIndexes( group )
{
   var count = group.length/2;
   var nodeIndexes = [];
   for(var i = 0; i < count; i++)
   {  
      var num = Math.floor( Math.random() * (group.length - 1) + 1 );
      while( nodeIndexes.indexOf( num ) !== -1 )
      {
         num = Math.floor( Math.random() * (group.length - 1) + 1 );
      }
      nodeIndexes.push( num );
   }
   return nodeIndexes;
}

function getMinorityNodeIndexes( group )
{
   var nodeIndexes = [];
   var count = group.length/2 - 1;
   for(var i = 0; i < count; i++)
   {
      var num = Math.floor( Math.random() * (group.length - 1) + 1 );
      while( nodeIndexes.indexOf( num ) !== -1 )
      {
         num = Math.floor( Math.random() * (group.length - 1) + 1 );
      }
      nodeIndexes.push( num );
   }
   return nodeIndexes;
}

function createGroupAndNode( db, rgName, hostName, nodesNum)
{
   try
   {
      var rg = db.createRG( rgName ) ;
      var failedCount = 0;
      for( var i = 0;i < nodesNum;i++ )
      {
         var svc = parseInt( RSRVPORTBEGIN ) + 10 * ( i + failedCount ) ;
         var dbPath = RSRVNODEDIR + "data/" + svc ;
         var checkSucc = false;
         var times = 0;
         var maxRetryTimes = 10;
         do
         {
            try
            {
               rg.createNode( hostName, svc, dbPath, {diaglevel:5} ) ;
               println( "create node: " + hostName + ":" + svc + " dbpath: " + dbPath ) ;
               checkSucc = true;
            }
            catch( e )
            {
               //-145 :SDBCM_NODE_EXISTED  -290:SDB_DIR_NOT_EMPTY
               if( e == -145 || e == -290 )
               {
                  svc = svc + 10;
                  dbPath = RSRVNODEDIR + "data/" + svc;
                  failedCount++;
               }
               else
               {
                  throw "create node failed!  port = " + svc + " dataPath = " + dbPath + " errorCode: " + e;
               }
               times++;
            }
         }
         while(!checkSucc && times < maxRetryTimes);
      }
      println( "start group" ) ;
      rg.start() ;
   }
   catch(e)
   {
      throw new Error(e);
   }
}

function getLSN( node )
{
   try
   {
      var db = new Sdb( node.HostName, node.svcname );
      var cursor = db.snapshot( SDB_SNAP_DATABASE );
      var completeLSN = cursor.current().toObj()["CompleteLSN"];
      return completeLSN;
   }
   catch(e)
   {
      throw new Error(e);
   }
   finally
   {
      if( db !== null )
      {
         db.close();
      }
   }
} 

function waitSync(masterNode, slaveNode)
{
   var doTimes = 0;
   var timeout = 300;
   var isSync = false;
   while( doTimes < timeout )
   {
      var masterNodeLSN = getLSN( masterNode );
      var slaveNodeLSN = getLSN( slaveNode );
      if( masterNodeLSN > slaveNodeLSN )
      {
         sleep(1000);
         doTimes++;
      }
      else
      {
         isSync = true;
         break;
      }
   }
   if( !isSync )
   {
      throw new Error( "wait sync failed!" );
   }
}
 
function checkReelect( groupName, hostName, svcName)
{
   var masterNode = db.getRG(groupName).getMaster();
   var masterNodeHostName = masterNode.getHostName();
   var masterNodeSvcName = masterNode.getServiceName();
   if( masterNodeHostName != hostName || masterNodeSvcName != svcName )
   {
      throw new Error( "Reelect failed!" );
   }
}                             
