import( "../lib/main.js" )
import( "../lib/basic_operation/commlib.js" )

/************************************************************************
*@Description: 获取满足指定节点数目的组
*@author:      zhaoxiaoni
*@createDate:  2018.1.22
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
*@author:      wuyan
*@createDate:  2018.1.22
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
*@Description: get svcname of the data group
*@author:      wuyan
*@createDate:  2018.1.22
**************************************************************************/
function getGroupNodes( groupName )
{   
   var groupInfo = db.getRG(groupName).getDetail().current().toObj().Group;  
   var groupNodes = [];
   for( var i in groupInfo )
   {
      var nodeInfo = groupInfo[i].HostName + ":" + groupInfo[i].Service[0].Name;
      groupNodes.push( nodeInfo );
   }  
   return groupNodes;
}

/************************************************************************
*@Description: 更新节点配置
*@author:      wuyan
*@createDate:  2018.1.22
**************************************************************************/
function updateConf ( db, configs, options, errno )
{
   try
   {
      db.updateConf( configs, options );
   }
   catch( e )
   {
      if( errno === undefined || e.message !== errno.toString() )
      {
         throw e;
      }
   }
}

function deleteConf ( db, configs, options, errno )
{
   try
   {
      db.deleteConf( configs, options );
   }
   catch( e )
   {
      if( errno === undefined || e.message !== errno.toString() )
      {
         throw e;
      }
   }
}

/************************************************************************
*@Description: 检查访问节点是否符合预期
*@author:      wuyan
*@createDate:  2018.1.22
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
*@createDate:  2019-11-27
**************************************************************************/
function sortBy( props )
{
   return function( a, b )
   {
       return a[props] - b[props];
   }
}
