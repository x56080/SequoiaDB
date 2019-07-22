/************************************
*@Description: insert data
*@author:      wuyan
*@createDate:  2018.1.22
**************************************/
function insertData( dbcl, insertNum)
{
   if( undefined == this.number ){ this.number = 5000 ; }
   try
   {
      var docs = [];
      for( var i = 0; i < number; ++i )
      {      
         var no = i;
         var a = i;
         var user = "test"+i;
         var phone = 13700000000+i;
         var time = new Date().getTime(); 
         var doc = {no:no, a:a,customerName:user, phone:phone, openDate:time};      
         //data example: {"no":5, customerName:"test5", "phone":13700000005, "openDate":1402990912105
         
         docs.push( doc );
      }	
      dbcl.insert( docs );       
   }
   catch(e)
   {
      throw buildException("insertData()",e,"insert", "insert success","insert fail");
   }
}

/************************************
*@Description: get the query Access node
*@author:      wuyan
*@createDate:  2018.1.22
**************************************/
function getAccessNode(dbcl)
{
   var actAccessNode = "";  
   var rc = dbcl.find().explain();  
   while( rc.next() )
   {      
      var atcObj = rc.current().toObj();
      var actAccessNode = atcObj.NodeName;
   } 
   return actAccessNode;  
}

/************************************
*@Description: get the query Access node for many groups
*@author:      wuyan
*@createDate:  2018.1.22
**************************************/
function getAllAccessNode(dbcl)
{
   try
   {
      var actAccessNode = {};  
      var rc = dbcl.find().explain();  
      while( rc.next() )
      {      
         var atcObj = rc.current().toObj();
         var accessNode = atcObj.NodeName;
         var groupName = atcObj.GroupName;
         actAccessNode[groupName] = accessNode ;
      } 
   }
   catch(e)
   {       
      throw buildException("getAllAccessNode()",e,"", "success","getNode fail,e="+e);
   }
   
   
   return actAccessNode;  
}

/************************************
*@Description: check the acess Nodeinfo
*@author:      wuyan
*@createDate:  2018.1.22
**************************************/
function checkAcessNodeResult( queryNode, expNodeName )
{
     
   if ( typeof(expNodeName) == "object")
   {     
      var inArryFlag =  isInArray(expNodeName, queryNode);      
      if( ! inArryFlag )
         throw buildException( "check AccessNode(object)", "", "get 'NodeName'", expNodeName, queryNode);     
   }        
   
   else if ( typeof(expNodeName) == "string")
   {
      if( queryNode !== expNodeName )
            throw buildException( "check AccessNode(string)", "", "get 'NodeName'", expNodeName, queryNode );     
   }
   else
   {
      throw buildException("checkAcessNodeResult()", "invalid parameter " ); 
   }  
}

/************************************
*@Description: get the all access node for many groups ,than check the acess Nodeinfo for many groups
*@author:      wuyan
*@createDate:  2018.1.22
**************************************/
function checkAllAcessNode( dbcl, queryNode, expNodeName )
{ 
   if( !queryNode.sort().toString() == expNodeName.sort().toString() )   
            throw buildException( "check AllAccessNode", "", "get 'NodeName'", 
                  expNodeName.toString(), queryNode.toString() );       
}

/************************************
*@Description: determines whether the value exists in an array
*@author:      wuyan
*@createDate:  2018.1.22
**************************************/
function isInArray(arry, checkValue)
{
   for( var i = 0; i < arry.length; i++)
   {
      if ( arry[i] == checkValue )
      {
         return true;
      }
   }
   return false;
}

/************************************
*@Description: get Session, then check get sessionInfo result
*@author:      wuyan
*@createDate:  2018.1.22
**************************************/
function getSessionAndCheckResult(db, expSessionInfo)
{
   var getSessionInfo = db.getSessionAttr().toObj();  
  
   if( JSON.stringify(getSessionInfo.PreferedInstance) !== JSON.stringify(expSessionInfo.PreferedInstance) )
         throw buildException( "check AccessInfo", "", "get 'PreferedInstance'", expSessionInfo.PreferedInstance, getSessionInfo.PreferedInstance );     
   
   if( getSessionInfo.PreferedInstanceMode !== expSessionInfo.PreferedInstanceMode )
         throw buildException( "check AccessInfo", "", "get 'PreferedInstanceMode'", 
                               expSessionInfo.PreferedInstanceMode, getSessionInfo.PreferedInstanceMode );     
   
   if( getSessionInfo.Timeout !== expSessionInfo.Timeout )
         throw buildException( "check AccessInfo", "", "get 'Timeout'", 
         expSessionInfo.Timeout, getSessionInfo.Timeout );     
   
}

/************************************
*@Description: get svcname of the data group
*@author:      wuyan
*@createDate:  2018.1.22
**************************************/
function getSvcNameList(db,groupName)
{   
   var groupInfo = db.getRG(groupName).getDetail().current().toObj().Group;  
   var svcNameList = [];
   
   for( var i in groupInfo )
   {
      var nodeInfo = groupInfo[i].HostName + ":" + groupInfo[i].Service[0].Name;
      svcNameList.push( nodeInfo );
   }  
   
   return svcNameList;
}

/************************************
*@Description: create group, then create node by the group
*@author:      wuyan
*@createDate:  2018.1.22
**************************************/
function createRGAndNode(db, groupName, instanceidList, nodeNum)
{
   if ( typeof(nodeNum) == "undefined" ) { nodeNum = 3 ; }
   if ( typeof(instanceidList) == "undefined" ) { instanceidList = [0,0,0] ; }
   
   try
   {
      var rg = db.createRG(groupName);
      var nodeHostName = db.listReplicaGroups().current().toObj().Group[0].HostName;
      var nodeList = [];
      var failedCount = 0;
      for( var i = 0; i < nodeNum; i++)
      {
         var nodeService = parseInt( RSRVPORTBEGIN ) + 10 * ( i + failedCount ) ;
         var nodePath = RSRVNODEDIR + "data/" + nodeService ;
         var checkSucc = false;
         var times = 0;
         var maxRetryTimes = 10;
         
         //first create node is master node 
         if (i == 0)
         {
            var config = { instanceid : instanceidList[i], weight:100,diaglevel:5 };
         } 
         else
         {
            var config = { instanceid : instanceidList[i],diaglevel:5};  
         }
         
         do
         {
            try
            {
               rg.createNode(nodeHostName, nodeService, nodePath, config);
               checkSucc = true;
               var logSourcePath = nodeHostName+":"+CMSVCNAME+"@"+nodePath+"/diaglog/sdbdiag.log";
               nodeList.push({"hostname": nodeHostName, "svcname": nodeService, "logSourcePath" : logSourcePath});
            }
            catch( e )
            {
               //-145 :SDBCM_NODE_EXISTED  -290:SDB_DIR_NOT_EMPTY
               if( e == -145 || e == -290 )
               {
                  nodeService = nodeService + 10;
                  nodePath = RSRVNODEDIR + "data/" + nodeService;
                  failedCount++;
               }
               else
               {
                  throw "create node failed!  nodeService = " + nodeService + " dataPath = " + nodePath + " errorCode: " + e;
               }
               times++;
            }
         }
         while(!checkSucc && times < maxRetryTimes);
      }
      println("start data group..");
      rg.start();
      
      //waiting for the success of the vote
      checkMasterExist( groupName );
      
      return nodeList;
   }
   catch(e)
   {       
      throw buildException("createRGAndNode()",e,"", "create success","createRGAndNode fail,e="+e);
   }
}

/************************************
*@Description: create second group , then create node by the group
*@author:      wuyan
*@createDate:  2018.1.22
**************************************/
function createSecondRGAndNode(db, groupName, instanceidList, nodeNum)
{
   if ( typeof(nodeNum) == "undefined" ) { nodeNum = 3 ; }
   if ( typeof(instanceidList) == "undefined" ) { instanceidList = [0,0,0] ; }
   
   try
   {
      var rg = db.createRG(groupName);      
      var nodeHostName = db.listReplicaGroups().current().toObj().Group[0].HostName;
      for( var i = 0; i < nodeNum; i++)
      {         
         var nodeService = parseInt(RSRVPORTBEGIN) + 100 + i *10;         
         var nodePath = RSRVNODEDIR + nodeService; 
         //first create node is master node 
         if (i == 0)
         {
            var config = { instanceid : instanceidList[i], weight:100,diaglevel:5 };
         } 
         else
         {
            var config = { instanceid : instanceidList[i],diaglevel:5};  
         }
                
         rg.createNode(nodeHostName, nodeService, nodePath, config);         
      }
      rg.start();
      //waiting for the success of the vote
      checkMasterExist( groupName );
   }
   catch(e)
   {       
      throw buildException("createSecondRGAndNode()",e,"", "create success","createRGAndNode fail,e="+e);
   }
}

function checkMasterExist( groupName )
{
   try
   {   
      var sleepInteval=10;
      var sleepDuration=0;
      var maxSleepDuration=60000; 
      var rc = db.exec("select IsPrimary,NodeName from $SNAPSHOT_SYSTEM where GroupName='" + groupName + "' and IsPrimary=true ");
      var num = rc.size();  
      while( num != 1 && sleepDuration < maxSleepDuration )
      {
            sleep( sleepInteval );
            sleepDuration += sleepInteval;   
            var rc = db.exec("select IsPrimary,NodeName from $SNAPSHOT_SYSTEM where GroupName='" + groupName + "' and IsPrimary=true ");
            var num = rc.size(); 
      }           
   } 
   catch( e )
   {
      throw e;
   }
}

/************************************
*@Description: check whether specified nodes is randomly selected,judging by the number of key
               the key is query node
*@author:      wuyan
*@createDate:  2018.1.22
**************************************/
function checkRandomAccessResult( expAccessNode, accessCount)
{
   var statSvcNameNum = Object.getOwnPropertyNames(accessCount).length;
   if( expAccessNode.length !== statSvcNameNum )
   {
      throw buildException( "check random AccessNode", "", "get accesscount", 
                                                expAccessNode.length, statSvcNameNum);
   }   
}

function storageNodeAccessCount(svcName, accessCount)
{
   if ( accessCount[svcName] == undefined )
   {
      accessCount[svcName] = 1;            
   }
   else
   {
       accessCount[svcName]++;      
   }
}

/************************************
*@Description: check the node is master or slave               
*@author:      wuyan
*@createDate:  2018.1.22
**************************************/
function checkAccessNodeIsPrimary( accessNode, groupName, isPrimary )
{   
   try
   {
      var isPrimaryFlag = new Sdb(accessNode).snapshot(7).current().toObj().IsPrimary;
      if( isPrimaryFlag !== isPrimary )
      {
         throw buildException("checkAccessNodeIsPrimary", null, accessNode+" isPriamry status",
									isPrimary, isPrimaryFlag);
      }
   }
   catch(e)
   {
      throw e;
   }  
}

function splitCL(dbcl, srcGroupName, dstGroupName)
{
   try
   {
      var percent = 50;
      dbcl.split( srcGroupName, dstGroupName, percent );
   }
   catch(e)
   {
      throw buildException("splitcl", null, "split fail!",
									srcGroupName, dstGroupName+"  e:"+e);
   } 
}