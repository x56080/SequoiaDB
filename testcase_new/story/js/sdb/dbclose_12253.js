/******************************************************************************
*@Description : test db operation after close
*               TestLink : seqDB-12253 调用close后执行操作
*@auhor       : Liang XueWang
******************************************************************************/
function main()
{
   var tmpdb = new Sdb( COORDHOSTNAME, COORDSVCNAME ) ;
   tmpdb.close() ;
   
   try
   {
       tmpdb.traceResume() ;
   }
   catch( e )
   {
       if( e == 0 )
       {
           throw buildException( "main", e, "test traceResume after close",
                 "no connection handle", errmsg ) ;
       }
   }
}

main() ;
