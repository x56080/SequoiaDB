/*******************************************************************************
*@Description :   普通表 设置从1-7节点上读
*@Modify List :   2016-03-17  Ting YU Modify
*******************************************************************************/
main();

function main()
{  
   try
   {
      if( commIsStandalone(db) )
      {  
         println(" Deploy mode is standalone!");
         return;
      }
      
      var csName = COMMCSNAME;
      var clName = COMMCLNAME;
      
      // create
      var clObj = new Collection( csName, clName, {ReplSize:0} );
      var cl = clObj.create();
      var groupName = clObj.getGroups()[0];
      var nodeList = getNodeList( groupName );

      // insert
      var recs = [{a:1},{a:2},{a:3},{a:4}];  
      insertAnotherSession( csName, clName, recs );               
      
      // query
      var instances = [ 1, 2, 3, 4, 5, 6, 7 ]; 
      for( var i in instances )   
      {
         setAndcheck( cl, instances[i], nodeList );
      }
   }
   catch( e )
   {
      throw e ;
   }
}

function setAndcheck( cl, instance, nodeList )
{
   println( "---begin to set PreferedInstance: " + instance );     
   db.setSessionAttr( { "PreferedInstance": instance } );  
             
   var queryNode = cl.find().explain({Run:true}).current().toObj().NodeName;

   var expNode = nodeList[ ( instance - 1 ) % nodeList.length ];
   if( queryNode !== expNode )
   {
      throw buildException( "setAndcheck()", null, "explain", expNode, queryNode );
   }
}

function getNodeList( groupName )
{
   var nodeList =[];
   var groupInfo = db.getRG(groupName).getDetail().current().toObj().Group;
   for( var i in groupInfo )
   {
      var nodeInfo = groupInfo[i].HostName + ":" + groupInfo[i].Service[0].Name;
      nodeList.push( nodeInfo );
   }
   println( "---get nodelist of " + groupName + ": " + nodeList );
   
   return nodeList;
}

