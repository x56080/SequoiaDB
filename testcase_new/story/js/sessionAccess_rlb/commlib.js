/************************************************************************
*@Description: 获取满足指定节点数目的组
*@author:      Zhao xiaoni
*@createDate:  2020-4-15
**************************************************************************/
function getGroupsWithNodeNum( nodesNum )
{
   var groups = [];
   var groupArray = commGetGroups( db );
   for( var i = 0; i < groupArray.length; i++ )
   {
      var group = groupArray[i];
      if( group.length - 1 >= nodesNum )
      {
         groups.push( group );
      }
   }
   return groups;
}

/************************************************************************
*@Description: insert data
*@author:      Zhao xiaoni
*@createDate:  2020-4-15
**************************************************************************/
function insertData( cl )
{
   var docs = [];
   for( var i = 0; i < 5000; ++i )
   {      
      docs.push( { a: i } );
   }	
   cl.insert( docs );       
}

/* *******************************************************************
@Description: create group and nodes
              db: connection handle, can't be standalone                
              groupName: group name
              nodesNum: node num, node svc like 26000 26010 ....
@return       nodeInfos : hostname, svcname
@author: Zhao xiaoni 
******************************************************************* */
function createRGAndNodes( db, groupName, hostName, nodeNum, instanceidList )
{
   var rg = db.createRG( groupName );   
   
   var maxRetryTimes = 100;
   var nodeNames = [];
   for( var i = 0; i < nodeNum; i++ )
   {
      var failedCount = 0;
      var svcName = parseInt( RSRVPORTBEGIN ) + 10 * ( i + failedCount );
      var dbPath = RSRVNODEDIR + "data/" + svcName;
      do
      {
         try
         {
            new Remote( hostName ).getCmd().run( "lsof -i:" + svcName );
            svcName = svcName + 10;
            dbPath = RSRVNODEDIR + "data/" + svcName;
            failedCount++;
            continue;
         }
         catch( e )
         { 
            if( e !== 1 )
            {
               throw new Error( "lsof check port error: " + e ); 
            }
         }

         //first create node is master node
         var instanceid = 0;
         instanceidList[i] == undefined ? instanceid = 0 : instanceid = instanceidList[i];
         i == 0 ? config = { "instanceid" : instanceid, weight: 100, diaglevel: 5 } : config = { "instanceid" : instanceid, diaglevel: 5 };

         try
         {
            rg.createNode( hostName, svcName, dbPath, config );
            println( "create node: " + hostName + ":" + svcName + " dbpath: " + dbPath );
            nodeNames.push( hostName + ":" + svcName );
            break;
         }
         catch( e )
         {
            //-145 :SDBCM_NODE_EXISTED  -290:SDB_DIR_NOT_EMPTY
            if( e == -145 || e == -290 )
            {
               svcName = svcName + 10;
               dbPath = RSRVNODEDIR + "data/" + svcName;
               failedCount++;
            }
            else
            {
               throw new Error( "create node failed!  port = " + svcName + " dataPath = " + dbPath + " errorCode: " + e );
            }
         }
      }
      while( failedCount < maxRetryTimes );
   }
   rg.start();

   //waiting for the success of the vote
   checkMasterExist( groupName );

   return nodeNames;
}

function checkMasterExist( groupName )
{
   try
   {
      var doTimes = 0;
      var totalTimes = 6000; 
      var rc = db.exec("select IsPrimary,NodeName from $SNAPSHOT_SYSTEM where GroupName='" + groupName + "' and IsPrimary=true ");
      var num = rc.size();  
      while( num != 1 && doTimes < totalTimes )
      {
            sleep( 100 );
            doTimes++;   
            var rc = db.exec("select IsPrimary,NodeName from $SNAPSHOT_SYSTEM where GroupName='" + groupName + "' and IsPrimary=true ");
            var num = rc.size(); 
      }           
   } 
   catch( e )
   {
      throw new Error( e );
   }
}

/************************************************************************
*@Description: 检查访问节点是否符合预期
*@author:      Zhao xiaoni
*@createDate:  2020-4-15
**************************************************************************/
function checkAccessNodes( cl, expAccessNodes, options )
{
   var doTimes = 0;
   var timeOut = 10000;
   var actAccessNodes = [];
   while( doTimes < timeOut )//设置instanceid后，获取访问的节点，当访问节点数组的长度等于期望结果时结束循环
   { 
      db.setSessionAttr( options );
      var cursor = cl.find().explain();
      while( cursor.next() )
      {
         var actAccessNode = cursor.current().toObj().NodeName;
         if( actAccessNodes.indexOf( actAccessNode ) === -1 )
         {
            actAccessNodes.push( actAccessNode );
         }
      }

      if( actAccessNodes.length === expAccessNodes.length )
      {  
         break;
      }
      else
      { 
         sleep( 10 ); 
         doTimes++;
      }
   }

   if( doTimes >= timeOut )
   {
      throw new Error( "actAccessNodes: " + actAccessNodes + ", expAccessNodes: " + expAccessNodes );
   }

   //实际结果与预期结果比较
   for( var i in expAccessNodes )
   {
      if( actAccessNodes.indexOf( expAccessNodes[i] ) === -1)
      {
         println("actAccessNodes: "+actAccessNodes+"\nexpAccessNodes: " + expAccessNodes);
         throw new Error( "The actAccessNodes do not include the node: " + expAccessNodes[i] );
      }
   }
}

/************************************************************************
*@Description: 按照属性排序
*@author:      Zhao xiaoni
*@createDate:  2020-4-15
**************************************************************************/
function sortBy( props )
{
   return function( a, b )
   {
       return a[props] - b[props];
   }
}

