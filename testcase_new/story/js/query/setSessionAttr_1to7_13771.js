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
      
      var clObj = new Collection( csName, clName, {ReplSize:0} );
      var cl = clObj.create();
      var groupName  = clObj.getGroups()[0];
      
      var recs = [{a:1},{a:2},{a:3},{a:4}];  
      insertAnotherSession( csName, clName, recs );               
      
      var hasSlave = checkSlaveExist( groupName );
      
      var queryNodes = testSet1to7( cl, recs ); 
      checkQueryNodes( groupName, queryNodes, hasSlave );        
   }
   catch( e )
   {
      throw e ;
   }
}

function testSet1to7( cl, recs )
{
   var nodeList = [];
   var opts = [ 1, 2, 3, 4, 5, 6, 7 ]; 
   
   for( var i in opts )   
   {
      var opt = opts[i];
      
      println( "---begin to set PreferedInstance: " + opt );     
      db.setSessionAttr({"PreferedInstance":opt});  
                
      var queryNode = cl.find().explain({Run:true}).current().toObj().NodeName;
      nodeList.push( queryNode );
      
      var rc = cl.find().sort({a:1});
      checkRec( rc, recs );
   }
         
   return nodeList;
}

function checkQueryNodes( groupName, queryNodes, hasSlave )
{
   println("---begin to check query nodes");
   
   expRole = !hasSlave;  
   var rc = db.exec( "select NodeName from $SNAPSHOT_SYSTEM where GroupName='" + 
                     groupName + "' and IsPrimary=" + expRole );
   var expNodes = [];
   while( rc.next() )
   {
      var node = rc.current().toObj().NodeName;
      expNodes.push( node );
   }
   
   queryNodes = arrayUnique( queryNodes );
   if( JSON.stringify(queryNodes.sort()) !== JSON.stringify(expNodes.sort()) )
   {
      throw buildException( "checkQueryNodes()", null, "queryNodes",
   							    expNodes, queryNodes );
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

//数组去掉重复元素
function arrayUnique( arr )
{
   arr = arr.sort();
   
   var uniqueArr = [ arr[0] ];
   for( var i = 1; i < arr.length; i++ )
   {
      if( arr[i] !== uniqueArr[ uniqueArr.length-1] )
      {
         uniqueArr.push( arr[i] );
      }
   }
   
   return uniqueArr;
}