/*******************************************************************************
*@Description : seqDB-7526::shell_输入strict格式，查询显示
seqDB-7527::shell_输入js格式，查询显示
*@Modify List : 2016-3-28  Ting YU  Init
*******************************************************************************/
main(); 

function main()
{
   try
   {
      var csName = COMMCSNAME; 
      var clName = COMMCLNAME; 
      
      var clObj = new Collection( csName, clName, {ReplSize:0} ); 
      var cl = clObj.create(); 
      
      testStrictFormat( cl ); 
      testJSFormat( cl ); 
      
   }
   catch( e )
   {
      throw e; 
   }
}

function testStrictFormat( cl )
{
   println( '---begin to check strict format: {$numberLong:"2147483647"}' ); 
   cl.remove(); 
   
   var val = 2147483647; 
   var rec = {a:{$numberLong:val.toString()}}; 
   cl.insert( rec ); 
   
   var rc = cl.find(); 
   var expRec = {a:2147483647}; 
   checkRec( rc, [expRec] ); 
   
   println( '---begin to check strict format: {$numberLong:"9007199254740992"}' ); 
   cl.remove(); 
   
   var val = 9007199254740992; 
   var rec = {a:{$numberLong:val.toString()}}; 
   cl.insert( rec ); 
   
   var rc = cl.find(); 
   var expRec = rec; 
   checkRec( rc, [expRec] ); 
}

function testJSFormat( cl )
{
   println( '---begin to check JS format: NumberLong( "-2147483647" )' ); 
   cl.remove(); 
   
   var val = -2147483647; 
   var rec = {a:NumberLong( val.toString() )}; 
   cl.insert( rec ); 
   
   var rc = cl.find(); 
   var expRec = {a:val}; 
   checkRec( rc, [expRec] ); 
   
   println( '---begin to check JS format: NumberLong( -2147483647 )' ); 
   cl.remove(); 
   
   var val = -2147483648; 
   var rec = {a:NumberLong( val )}; 
   var expRec = {a:val}; 
   cl.insert( rec ); 
}
