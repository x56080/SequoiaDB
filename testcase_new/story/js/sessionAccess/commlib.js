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

/************************************************************************
*@Description: get node name of the data group
*@author:      Zhao xiaoni 
*@createDate:  2020-4-15
**************************************************************************/
function getGroupNodes( groupName )
{   
   var groupInfo = db.getRG( groupName ).getDetail().current().toObj().Group;  
   var groupNodes = [];
   for( var i in groupInfo )
   {
      var nodeInfo = groupInfo[i].HostName + ":" + groupInfo[i].Service[0].Name;
      groupNodes.push( nodeInfo );
   }  
   return groupNodes;
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

