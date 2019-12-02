/*******************************************************************************
*@Description : 7528:shell_使用strict格式运算
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
      
      testEqual( cl ); 
      testArithmetic( cl ); 
   }
   catch( e )
   {
      throw e; 
   }
}

function testEqual( cl )
{
   println( '---begin to test equal' ); 
   cl.remove(); 
   
   var val = 9007199254740992; //2^53
   var rec = {a:{$numberLong:val.toString()}}; 
   cl.insert( rec ); 
   
   var rc = cl.find(); 
   var queryVal = rc.current().toObj().a; 
   
   if( queryVal == val )
   {
        //ok
   }
   else
   {
      throw buildException( "check value", "", "compare " + val + " with " + JSON.stringify( queryVal ), 
      "equal", "not equal" ); 
   }
   
   println( '---begin to test not equal' ); 
   if( queryVal !=( val-1 ) )
   {
        //ok
   }
   else
   {
      throw buildException( "check value", "", "compare " +( val-1 )+ " with " + JSON.stringify( queryVal ), 
      "not equal", "equal" ); 
   }
}

function testArithmetic( cl )
{
   var val = 9007199254740991; 
   
   cl.remove(); 
   cl.insert( {a:{$numberLong:val.toString()}} ); 
   var queryVal = cl.find().current().toObj().a; 
   
   println( "---begin to test add '+' divide '/' operation" ); 
   var actVal =( queryVal + 1 )/ 2; 
   var expVal =( val + 1 )/ 2; 
   if( actVal !== expVal )
   throw buildException( "check value", "", "( " + JSON.stringify( actVal )+ " + 1 )/ 2", 
   "equal to " + expVal, "not equal" ); 
   
   println( "---begin to test subtract '-' multiply '*' operation" ); 
   var actVal =( queryVal - 9007199254740990 )* 2; 
   var expVal =( val      - 9007199254740990 )* 2; 
   if( actVal !== expVal )
   throw buildException( "check value", "", "( " + JSON.stringify( actVal )+ " - 9007199254740990 )* 2", 
   "equal to " + expVal, "not equal" ); 
   
   println( "---begin to test mod '%' operation" ); 
   var actVal = queryVal % 2; 
   var expVal = val      % 2; 
   if( actVal !== expVal )
   throw buildException( "check value", "", "( " + JSON.stringify( actVal )+ "% 2", 
   "equal to " + expVal, "not equal" ); 
   
   println( "---begin to test abs '||' operation" ); 
   var actVal = Math.abs( queryVal * -0.5 ); 
   var expVal = Math.abs( val      * -0.5 ); 
   if( actVal !== expVal )
   throw buildException( "check value", "", "Math.abs( " + JSON.stringify( actVal )+ " * -0.5 )", 
   "equal to " + expVal, "not equal" ); 
}

