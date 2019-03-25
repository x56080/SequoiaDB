/*******************************************************************************
*@Description :  common function
*@Modify list :
*                2019/3/19  XiaoNi Huang Init
*******************************************************************************/


/* ****************************************************
@Description: clean cl in the pre-condition, than create cl
@Return: cl
@Author: XiaoNi Huang
**************************************************** */
function readyCL( clName, options )
{
   println("\n---Begin to create CL.");
   commDropCL( db, COMMCSNAME, clName, true, true, 
            "Failed to drop CL in the pre-condition." );  
            
   if( options == "undefined" )
   {                   
      var cl = commCreateCL( db, COMMCSNAME, clName, -1, true, true, false,
                          "Failed to create CL." ); 
   }
   else
   {
      var cl = commCreateCLByOption(db, COMMCSNAME, clName, options, true, true);
   }   
   return cl;
}

/* ****************************************************
@Description: clean cl in the end-condition
@Author: XiaoNi Huang
**************************************************** */
function cleanCL( clName )
{
   println("\n---Begin to drop CL.");	
   commDropCL( db, COMMCSNAME, clName, false, false,
               "Failed to drop CL in the end-condition" );
}

/* ****************************************************
@Description: create data groups
@Return: dataGroupNames
@Author: XiaoNi Huang
**************************************************** */
function createDataGroups( hostName , groupNum, tmpGroupName, nodeNum )
{
   if( nodeNum === "undefined" ){ nodeNum = 1; }
   if( hostName === "localhost" || hostName === "127.0.0.1" ){ hostName = getHostName(); }
   var dataGroupNames = [];
   var tmpNum = 0;
   for(var i = 0; i < groupNum; i++)
   {
      var groupName = tmpGroupName + "_" + i;
      var rg = db.createRG( groupName );
      for( var j = 0; j < nodeNum; j++ )
      {
         var port = parseInt( RSRVPORTBEGIN ) + ( tmpNum * 10 );
         rg.createNode( hostName, port, RSRVNODEDIR +"data/"+ port );
         rg.start();
         tmpNum++;
      }
      dataGroupNames.push( groupName );
   }
   return dataGroupNames;
}

/* ****************************************************
@Description: get node's hostname
@Return: random hostname
@Author: XiaoNi Huang
**************************************************** */
function getHostName()
{
   var rg = db.getCoordRG();
   var node = rg.getSlave();
   var hostname = node.getHostName();
   return hostname;
}

/* ****************************************************
@Description: remove data groups
@Author: XiaoNi Huang
**************************************************** */
function removeDataGroups( dataGroupNames, ignoreRGNotExist )
{
   try
   {
      for(var i = 0 ; i < dataGroupNames.length; i++)
      {
         db.removeRG( dataGroupNames[i] );
      }
   }
   catch( e )
   {
      if( -154 == e && !ignoreRGNotExist)
      {
         throw e;
      }
   }
}