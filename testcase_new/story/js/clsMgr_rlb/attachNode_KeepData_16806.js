/* *****************************************************************************
@discretion: attachNode( )中KeepData参数校验
@author：2018-12-12 wangkexin
***************************************************************************** */
import ("../clsMgr/commlib.js");
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
	  
	  var hostname1 = db.getRG(groupName1).getDetail().next().toObj()["Group"][0]["HostName"];
	  var hostname2 = db.getRG(groupName1).getDetail().next().toObj()["Group"][0]["HostName"];
	  
	  var port = parseInt(RSRVPORTBEGIN) + 50;
	  
	  db.getRG(groupName1).createNode(hostname1, port, RSRVNODEDIR + port);
	  db.getRG(groupName1).start();
	  db.getRG(groupName1).detachNode(hostname1, port, {KeepData:true});
	  
	  println("begin to attach node");
	  //test a : KeepData设为合法值
	  db.getRG(groupName2).attachNode(hostname2, port, {KeepData:true});
	  db.getRG(groupName2).detachNode(hostname2, port, {KeepData:false});
	  
	  println("begin to attach with keepdata is '' ");
	  //test b : KeepData设为空值
	  try
	  {
		  db.getRG(groupName2).attachNode(hostname2, port, {KeepData:""});
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
		  db.getRG(groupName2).attachNode(hostname2, port, {KeepData:"test"});
		  throw "exp fail but found success";
	  }catch( e )
	  {
		  if ( e !== -6) 
		  {
			  throw buildException("attachNode with KeepData is 'test' fail", e); 
			}
	  }
   }
   catch( e )
   {
      throw buildException("check attachNode16806", e)
   }
   finally
   {
       println("----------clear node");
       try
       {
          db.getRG(groupName2).attachNode(hostname2, port, {KeepData:true});
       }
       catch( e )
       {
           // -145:SDBCM_NODE_EXISTED  -155:SDB_CLS_NODE_NOT_EXIST
           if( e !== -145 && e !== -155 )
           {
               throw e;
           }
       }
	   db.getRG(groupName2).start();
       try
       {
          db.getRG(groupName2).removeNode(hostname2, port);
       }
       catch( e )
       {
           if( e == -155 )
           {
               db.getRG(groupName1).removeNode(hostname2, port);
           }
           else
           {
               throw e;
           }
       }
       
	   if (db !== undefined)
	   {
		   db.close();      	      
		} 
	}   
}

