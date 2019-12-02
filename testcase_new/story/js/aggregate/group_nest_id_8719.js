function main()
{
   var cl = new collection( db, COMMCSNAME, COMMCLNAME ); 
   cl.create(); 
   cl.bulkInsert(); 
   var cursor = cl.execAggregate( {$group:{_id:"$info.sex"}} ); 
   var expectResult = [{"no": 1002, "score": 85, "interest": ["movie", "photo"], "major": "计算机软件与理论", "dep": "计算机学院", "info": {"name": "Holiday", "age": 22, "sex": "女"}}, 
   {"no": 1000, "score": 80, "interest": ["basketball", "football"], "major": "计算机科学与技术", "dep": "计算机学院", "info": {"name": "Tom", "age": 25, "sex": "男"}}]; 
   var ret = checkResult( cursor, expectResult ); 
   if( !ret[0] )
   {
      var parameter = "{$group:{_id:'$info.sex'}}"
      throw buildException( "main", 0, "cl.aggregate( " + parameter + " )", 
      JSON.stringify( ret[1] ), JSON.stringify( ret[2] ) ); 
   }
   
   cl.drop(); 
}

main()
