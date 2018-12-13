/* *****************************************************************************
@discretion: detachNode( )中KeepData参数校验
@author：2018-12-12 wangkexin
***************************************************************************** */

main(db);
function main(db)
{	  
	try
	{
	  if (commGetGroupsNum(db) < 2)
      {
         println("--least two groups");
         return ;
      } 
	  var groupList = getGroup(db);
	  var groupName = groupList[0];
	  var port = parseInt(RSRVPORTBEGIN) + 50;
	  
	  db.getRG(groupName).createNode(COORDHOSTNAME, port, RSRVNODEDIR+port);
	  db.getRG(groupName).start();
	  
	  println("begin to detach node");
	  //test a : KeepData设为合法值
	  db.getRG(groupName).detachNode(COORDHOSTNAME, port, {KeepData:true});
	  db.getRG(groupName).attachNode(COORDHOSTNAME, port, {KeepData:true});
	  
	  println("begin to detach with keepdata is '' ");
	  //test b : KeepData设为空值
	  try
	  {
		  db.getRG(groupName).detachNode(COORDHOSTNAME, port, {KeepData:""});
		  throw "exp fail but found success";
	  }catch( e )
	  {
		  if ( e !== -6) 
		  {
			  throw buildException("detachNode with KeepData is '' fail", e); 
			}
	  }
	  
	  println("begin to detach with keepdata is 'test' ");
	  //test c : KeepData设为非布尔值
	  try
	  {
		  db.getRG(groupName).detachNode(COORDHOSTNAME, port, {KeepData:"test"});
		  throw "exp fail but found success";
	  }catch( e )
	  {
		  if ( e !== -6) 
		  {
			  throw buildException("detachNode with KeepData is 'test' fail", e); 
			}
	  }
	  
	  println("----------clear node");
	  db.getRG(groupName).removeNode(COORDHOSTNAME, port);
	  
   }
   catch( e )
   {
      throw buildException("check detachNode16809", e)
   }finally
   {
	   if (db !== undefined)
	   {
		   db.close();      	      
		} 
	}   
}

