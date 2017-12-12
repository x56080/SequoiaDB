/*******************************************************************************
*@Description :   普通表 设置从任意节点上读
*@Modify List :   2016-03-17 Ting YU   Modify
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
      
      var clObj = new Collection( csName, clName, {ReplSize:0} );
      var cl = clObj.create();
      var groupName  = clObj.getGroups()[0];
      
      var recs = [{a:1},{a:2},{a:3},{a:4}];  
      insertAnotherSession( csName, clName, recs );               
      
      testSetAnyone( cl, groupName, recs );     
   }
   catch( e )
   {
      throw e ;
   }
}

function testSetAnyone( cl, groupName, recs )
{
   var opts = [ "A", "a" ]; 
   var opt = opts[ parseInt( Math.random() * opts.length ) ];
      
   println( "---begin to set PreferedInstance: " + opt );     
   db.setSessionAttr({"PreferedInstance":opt});  
             
   var queryNode = cl.find().explain({Run:true}).current().toObj().NodeName;
   checkQueryNode( queryNode, groupName );
   
   var rc = cl.find().sort({a:1});
   checkRec( rc, recs );     
}

function checkQueryNode( node, groupName )
{   
   println("---begin to check query node[" + node + "] is in group[" + groupName + "] or not");
   try
   {
      db.getRG(groupName).getNode(node);
   }
   catch(e)
   {
      throw buildException("checkRole()", null, "db.getRG("+groupName+").getNode("+node+")",
									"success", e);
   }
}