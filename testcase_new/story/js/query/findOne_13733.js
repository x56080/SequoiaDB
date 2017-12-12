/*******************************************************************************
*@Description : 验证findOne
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
      
      //insert records
      var clObj = new Collection( csName, clName, {ReplSize:0} );
      var cl = clObj.create();      
      clObj.insertRecs( [{a:1}, {a:1}, {a:2}] );
      
      //findOne without option
      println("---begin to findOne without option");               
      var rc = cl.findOne() ;
      var cnt = rc.toArray().length;
      if( cnt !== 1 )
      {
         throw buildException("check", "", "findOne().toArray().length", 1, cnt);
      }   
         
      //findOne with option
      println("---begin to findOne with option");                 
      var rc = cl.findOne({a:{$lte:1}}) ;
      var cnt = 0;
      while( rc.next() )
      {
         cnt++;
         var val = rc.current().toObj().a;
         if( val !== 1 )
         {
            throw buildException("check record", "", "findOne({a:{$lte:1}).current().toObj().a", 1, val);
         }
      }
      if( cnt !== 1 )
      {
         throw buildException("check count", "", "findOne({a:{$lte:1})", 1, cnt);
      }
      
   }
   catch( e )
   {
      throw e ;
   }
}