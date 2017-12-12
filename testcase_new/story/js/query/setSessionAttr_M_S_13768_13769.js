/*******************************************************************************
*@Description :   普通表 设置优先从主、备节点上读
*@Modify List :   2014-11-20  xiaojun Hu  changed
                  2016-03-17  Ting YU     Modify
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
               
      checkMasterExist( groupName );
      var hasSalve = checkSlaveExist( groupName );
      
      testSetMaster( cl, groupName, recs );
      testSetSlave ( cl, groupName, recs, hasSalve );
     
   }
   catch( e )
   {
      throw e ;
   }
}

function testSetMaster( cl, groupName, recs )
{
   var opts = [ "M", "m" ]; 
   var opt = opts[ parseInt( Math.random() * opts.length ) ];
      
   println( "---begin to set PreferedInstance: " + opt );     
   db.setSessionAttr({"PreferedInstance":opt});  
             
   var queryNode = cl.find().explain({Run:true}).current().toObj().NodeName;
   checkRole( queryNode, groupName, true );
   
   var rc = cl.find().sort({a:1});
   checkRec( rc, recs );     
}

function testSetSlave( cl, groupName, recs, hasSalve )
{  
   var expMaster = false;
   if( hasSalve === false )   expMaster = true;
      
   var opts = [ "S", "s" ]; 
   var opt = opts[ parseInt( Math.random() * opts.length ) ];
      
   println( "---begin to set PreferedInstance: " + opt );     
   db.setSessionAttr({"PreferedInstance":opt});  
             
   var queryNode = cl.find().explain({Run:true}).current().toObj().NodeName;
   checkRole( queryNode, groupName, expMaster );
   
   var rc = cl.find().sort({a:1});
   checkRec( rc, recs );     
}

function checkRole( node, groupName, expMaster )
{   
   println("---begin to check query node[" + node + "] is master or not");
   try
   {
      db.getRG(groupName).getNode(node);
   }
   catch(e)
   {
      throw buildException("checkRole()", null, "db.getRG("+groupName+").getNode("+node+")",
									"success", e);
   }
   
   var isMaster = new Sdb(node).snapshot(7).current().toObj().IsPrimary;
   if( isMaster !== expMaster )
   {
      throw buildException("checkRole()", null, node+" is master node",
									expMaster, isMaster);
   }
}

function checkMasterExist( groupName )
{
   println("---begin to make sure that group[" + groupName + "] has master node");
   
   var rc = db.exec("select IsPrimary,NodeName from $SNAPSHOT_SYSTEM where GroupName='" + groupName + "' and IsPrimary=true ");
   var num = rc.size();
   if( num !== 1 )
   {
      throw buildException( "masterExist()", null, "get master node",
   							    "1 node", num );
   }
}

function checkSlaveExist( groupName )
{
   println("---begin to make sure that group[" + groupName + "] has master node");
   
   var rc = db.exec("select IsPrimary,NodeName from $SNAPSHOT_SYSTEM where GroupName='" + groupName + "' and IsPrimary=false ");
   var num = rc.size();
   if( num === 0 )
   {
      return false;
   }
   else
   {
      return true;
   }
}