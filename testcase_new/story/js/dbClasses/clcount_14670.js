/*******************************************************************
* @Description : test case for CLCount
*                seqDB-14670:使用valueOf获取CLCount值
*                seqDB-14671:指定hint执行CLCount        
* @author      : Liang XueWang
*                2018-03-12
*******************************************************************/
var clName = COMMCLNAME + "_dbClasses14670" ;

try
{
   main();
}
catch(e)
{
   if ( e.constructor === Error )
   {
      println(e.stack) ;  
   }
   throw e ;
}
 ;

function main()
{
   var cl = commCreateCL( db, COMMCSNAME, clName) ;
   
   cl.insert( { a: 1 } ) ;
   cl.insert( { a: 2 } ) ;
   
   var cnt = cl.count().valueOf() ;
   if( cnt !== 2 )
   {
      throw new Error("expect: 2, actual: " + cnt );
   }
   
   cnt = cl.count().hint( { a: "" } ) ;
   if( parseInt( cnt ) !== 2 )
   {
      throw new Error("expect: 2, actual: " + cnt );
   }
   
   try
   {
      cl.count().hint( 1 ) ;  // nothrow
   }
   catch( e )
   {
      println( e ) ;
   }
   
   commDropCL( db, COMMCSNAME, clName ) ;
}