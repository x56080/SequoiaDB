/*******************************************************************************
*@Description : 1、在同一个session中，未关闭游标时，drop cs，可以删除成功
                2、在不同的session中，一个session未关闭游标时，另一个session删除cs，会报-147
*@Modify List : 2014-9-26   xiaojunHu  Init
                2016-3-17   Ting YU    modify
*******************************************************************************/
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
      
      //cursor not close
      var rc = db.getCS( csName ).getCL( clName ).find();
      rc.next();
      
      var hasDataContext = checkContext();
      
      //drop cs
      if( hasDataContext === true )
      {
         dropcsDiffSession( csName );
         dropcsSameSession( csName );                          
      }
      
   }
   catch( e )
   {
      throw e ;
   }
}

function checkContext()
{
   println( "---begin to check context is exits or not" );
   
   var sp = db.snapshot( SDB_SNAP_CONTEXTS_CURRENT );
   var hasDataContext = false;
   while( sp.next() && hasDataContext === false )
   {
      var ct = sp.current().toObj().Contexts;
      for( var i in ct )
      {
         var contextType = ct[i].Type;
         if( contextType == "DATA") 
         {
            hasDataContext = true;
            break;
         }
      }
   }
   println( "-----'DATA' Context = "+ hasDataContext );
   
   return hasDataContext;
}

function dropcsSameSession( csName )
{
   println( "---begin to drop cs in the same session" );
   db.dropCS( csName );
   
   println( "---begin to get cs" );  
   try
   {  
      db.getCS( csName );
      throw "did not throw error, expect throw -34";
   }
   catch(e)
   {
      if( e !== -34 )
      {
         throw e;
      }
   }
}

function dropcsDiffSession( csName )
{
   println( "---begin to drop cs in another session" );       
   try
   {   
      dbAnother = new Sdb( COORDHOSTNAME, COORDSVCNAME );
      dbAnother.dropCS( csName );
      throw "did not throw error, expect throw -147";
   }
   catch(e)
   {
      if( e !== -147 )
      {
         throw e;
      }
   }
   
   println( "---begin to get cs" );  
   db.getCS( csName );
}