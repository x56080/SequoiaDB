/************************************************************************
@Description : 关闭游标后，查看是否关闭了context
@Modify list : wang wenjing  Init
               Ting YU       modify
************************************************************************/
main();

function main()
{	  
	try
	{	
      var csName = COMMCSNAME;
      var clName = COMMCLNAME;
      
      //insert records > 128M
      var cl = new Collection( csName, clName, {ReplSize:0} );
      cl.create();
      cl.insert( 1000, "string", ['a', 'b', 'c'] );
      var preSize = getSessionContextsSize();     
      
      //cursor not close
      println("---begin to query and check context");
      var cursor = db.getCS( csName ).getCL( clName ).find();
      cursor.next();
      
      var hasCursorSize = getSessionContextsSize();
      if( hasCursorSize.toString() !== preSize.toString() ) 
      {
         println("before query context !== query context")
      }
      else
      {
         println("-----before query context === query context");
      }
      
      //close cursor
      println("---begin to close cursor and check context");
      cursor.close();
      
      var closeCursorSize = getSessionContextsSize();      
      if( closeCursorSize.toString() !== preSize.toString() ) 
      {
         throw buildException("check context", 0,"cursor.close()", "kill context", "Not kill context");
      }     
   }
   catch( e )
   {
      throw e ;
   }
}

function getSessionContextsSize()
{
    var contextSize = [];
   
    var rc = db.snapshot(SDB_SNAP_SESSIONS_CURRENT);
    while(rc.next())
    {
       var obj = rc.current().toObj();
       contextSize.push(obj["Contexts"].length);
    }
    return contextSize;
}
