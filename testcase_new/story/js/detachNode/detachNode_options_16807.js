/* *****************************************************************************
@discretion: detachNode( )options参数为空
@author：2018-12-12 wangkexin
***************************************************************************** */

main(db);
function main(db)
{	  
	try
	{	
	  var groupList = getGroup(db);
	  var groupName = groupList[0];
	  
      try
	  {
		  db.getRG(groupName).detachNode(COORDHOSTNAME,RSRVPORTBEGIN);
		  throw "exp fail but found success";
	  }catch( e )
	  {
		  if ( e !== -6) 
		  {
			  throw buildException("detachNode with options 'test' fail", e); 
			}
	  }
   }
   catch( e )
   {
      throw buildException("check detachNode16807", e)
   }finally
   {
	   if (db !== undefined)
	   {
		   db.close();      	      
		} 
	}   
}

