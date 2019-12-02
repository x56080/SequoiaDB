/*******************************************************************************
*@Description : Aggregate limit test
*@Modify list :
*               2016-03-18  wenjing wang rewrite
*******************************************************************************/
function testLimitMinBound( cl, docNumber )
{
   var cursor = cl.execAggregate( {$limit:-1} ); 
   var retNumber = getRetNumber( cursor )
   if( retNumber !== docNumber )
   {
      throw buildException( "main", 0, "cl.aggregate( {$limit:-1} )", 
      docNumber, retNumber ); 
   }
}

function testLimitMaxBound( cl, docNumber )
{
   var cursor = cl.execAggregate( {$limit:18} ); 
   var retNumber = getRetNumber( cursor ); 
   if( retNumber !== docNumber )
   {
      throw buildException( "main", 0, "cl.aggregate( {$limit:-3} )", 
      docNumber, retNumber ); 
   }
}

function testLimitAnyValue( cl )
{
   cursor = cl.execAggregate( {$limit:3} ); 
   var expectResult = [{no:1000, score:80, interest:["basketball", "football"], major:"计算机科学与技术", dep:"计算机学院", info:{name:"Tom", age:25, sex:"男"}}, 
   {no:1001, score:82, major:"计算机科学与技术", dep:"计算机学院", info:{name:"Json", age:20, sex:"男"}}, 
   {no:1002, score:85, interest:["movie", "photo"], major:"计算机软件与理论", dep:"计算机学院", info:{name:"Holiday", age:22, sex:"女"}}]
   var ret = checkResult( cursor, expectResult ); 
   if( !ret[0] )
   {
      throw buildException( "main", 0, "cl.aggregate( {$limit:3} )", 
      JSON.stringify( ret[1] ), JSON.stringify( ret[2] ) ); 
   }
}

function main()
{
   var cl = new collection( db, COMMCSNAME, COMMCLNAME ); 
   cl.create(); 
   var docNumber = cl.bulkInsert(); 
   testLimitMinBound( cl, docNumber ); 
   testLimitAnyValue( cl ); 
   testLimitMaxBound( cl, docNumber ); 
   cl.drop(); 
}
main()
