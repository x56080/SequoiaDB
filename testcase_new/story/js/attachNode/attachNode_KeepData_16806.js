/* *****************************************************************************
@discretion: attachNode( )中KeepData参数校验
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
	  var groupName1 = groupList[0];
	  var groupName2 = groupList[1];
	  var port = parseInt(RSRVPORTBEGIN) + 50;
	  
	  db.getRG(groupName1).createNode(COORDHOSTNAME, port, RSRVNODEDIR+port);
	  db.getRG(groupName1).start();
	  db.getRG(groupName1).detachNode(COORDHOSTNAME, port, {KeepData:true});
	  
	  println("begin to attach node");
	  //test a : KeepData设为合法值
	  db.getRG(groupName2).attachNode(COORDHOSTNAME, port, {KeepData:true});
	  db.getRG(groupName2).detachNode(COORDHOSTNAME, port, {KeepData:false});
	  
	  println("begin to attach with keepdata is '' ");
	  //test b : KeepData设为空值
	  try
	  {
		  db.getRG(groupName2).attachNode(COORDHOSTNAME, port, {KeepData:""});
		  throw "exp fail but found success";
	  }catch( e )
	  {
		  if ( e !== -6) 
		  {
			  throw buildException("attachNode with KeepData is '' fail", e); 
			}
	  }
	  
	  println("begin to attach with keepdata is 'test' ");
	  //test c : KeepData设为非布尔值
	  try
	  {
		  db.getRG(groupName2).attachNode(COORDHOSTNAME, port, {KeepData:"test"});
		  throw "exp fail but found success";
	  }catch( e )
	  {
		  if ( e !== -6) 
		  {
			  throw buildException("attachNode with KeepData is 'test' fail", e); 
			}
	  }
	  
	  println("----------clear node");
	  db.getRG(groupName2).attachNode(COORDHOSTNAME, port, {KeepData:true});
	  db.getRG(groupName2).start();
	  db.getRG(groupName2).removeNode(COORDHOSTNAME, port);
	  
   }
   catch( e )
   {
      throw buildException("check attachNode16806", e)
   }finally
   {
	   if (db !== undefined)
	   {
		   db.close();      	      
		} 
	}   
}

