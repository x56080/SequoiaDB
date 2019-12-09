/****************************************************
@description:	commlib of basic		
@modify list:
                     2015-4-3 Ting YU init
****************************************************/
var com = new Cmd();                 //com.run("command");
var info;                                    //returned information after curl command
var errno;                                	//errno in returned information
var infoSplit = new Array();		//split info to array 
var str;
/*****************************************************************
@description:	run CURL command, to get return errno by rest	
@input:		eg:curlPara=["cmd=query","name=foo.bar"]
				run("curl http://127.0.0.1:11814/ -d "cmd=query&name=foo.bar" 2>/dev/nu")
@expectation:	rest return message:{ "errno": 0 }{ "_id": { "$oid": "551d0486e9c0d31814000009" }, "age": 2 }
				info={ "errno": 0 }{ "_id": { "$oid": "551d0486e9c0d31814000009" }, "age": 2 }
				errno=0
				infosplit=[{ "errno": 0 },{ "_id": { "$oid": "551d0486e9c0d31814000009" }, "age": 2 }]				
@modify list:	
                     2015-3-30 Ting YU init
******************************************************************/
function runCurl ( curlPara )
{
   if( undefined == curlPara ) { throw "curlPara of runCurl couldn't be undefined!"; }
   if( 'string' == typeof ( COORDSVCNAME ) ) { COORDSVCNAME = parseInt( COORDSVCNAME, 10 ); }
   if( 'number' != typeof ( COORDSVCNAME ) ) { throw "COORDSVCNAME type, expect: number, actual: " + typeof ( COORDSVCNAME ); }
   str = "curl http://" + COORDHOSTNAME + ":" + eval( COORDSVCNAME + 4 ) + "/ -d '";
   for( var i = 0; i < curlPara.length; i++ )
   {
      str += curlPara[i];
      if( i < ( curlPara.length - 1 ) )
      {
         str += "&";
      }
   }
   str += "' 2>/dev/null";// to get curl command
   info = com.run( str );// to get info
   //	println("command: "+str+"\n"+"return: "+info);
   var tem = info.slice( 11, 18 );
   errno = parseInt( tem, 10 );// to get errno [int]

   infoSplit = info.replace( /\}\{/g, "\}\n\{" ).split( "\n" );
}

/*****************************************************************
@description:	open ssl, use https
@pre-condition: usessl=true in node config
@modify list:	
				2017-11-22 XiaoNi Huang init 
 *****************************************************************/
function runCurlByHttps ( curlPara )
{
   if( undefined == curlPara ) { throw "curlPara of runCurl couldn't be undefined!"; }
   if( 'string' == typeof ( COORDSVCNAME ) ) { COORDSVCNAME = parseInt( COORDSVCNAME, 10 ); }
   if( 'number' != typeof ( COORDSVCNAME ) ) { throw "COORDSVCNAME type, expect: number, actual: " + typeof ( COORDSVCNAME ); }
   str = "curl -k https://" + COORDHOSTNAME + ":" + eval( COORDSVCNAME + 4 ) + "/ -d '";
   for( var i = 0; i < curlPara.length; i++ )
   {
      str += curlPara[i];
      if( i < ( curlPara.length - 1 ) )
      {
         str += "&";
      }
   }
   str += "' 2>/dev/null";// to get curl command
   info = com.run( str );// to get info
   //	println("command: "+str+"\n"+"return: "+info);
   var tem = info.slice( 11, 18 );
   errno = parseInt( tem, 10 );// to get errno [int]

   infoSplit = info.replace( /\}\{/g, "\}\n\{" ).split( "\n" );
}

/*****************************************************************
@description:	run CURL command, if return errno != throwCond, throw out error	
@input:		curlPara=["cmd=create collectionspace","name=foo"]
				throwCond=[0,-33]
				throwOutput="Failed to create cs!"
@expectation:	if return errno == 0 or -33, NO error to be throwed out.
				if return errno !=0 and -33, throw "Faied to create cs!".
@modify list:	
                     2015-3-30 Ting YU init
******************************************************************/
function tryCatch ( curlPara, throwCond, throwOutput )
{
   if( undefined == curlPara )
   {
      println( "curlPara of tryCatch couldn't be undefined!" );
      throw "tryCatch message:" + throwOutput;
   }
   if( undefined == throwCond ) { throwCond = []; }
   if( undefined == throwOutput ) { throwOutput = ""; }
   var condFlag = true;
   try
   {
      runCurl( curlPara );
   } catch( e )
   {
      println( "Fail to runCurl()!" );
      println( "command: " + str + "\n" + "return: " + info );
      throw e;
   }

   try
   {
      for( var i = 0; i < throwCond.length; i++ )
      {
         condFlag = condFlag && errno != throwCond[i];
      }
      if( condFlag ) { throw info; }
   } catch( e )
   {
      println( "tryCatch message: " + throwOutput );
      println( "command: " + str + "\n" + "return: " + info );
      throw e;
   }
}

/***************************************************************
@description:	to get current function name, use https
@modify list:	
				2017-11-22 XiaoNi Huang init 
***************************************************************/
function tryCatchByHttps ( curlPara, throwCond, throwOutput )
{
   if( undefined == curlPara )
   {
      println( "curlPara of tryCatch couldn't be undefined!" );
      throw "tryCatchByHttps message:" + throwOutput;
   }
   if( undefined == throwCond ) { throwCond = []; }
   if( undefined == throwOutput ) { throwOutput = ""; }
   var condFlag = true;
   try
   {
      runCurlByHttps( curlPara );
   } catch( e )
   {
      println( "Fail to runCurl()!" );
      println( "command: " + str + "\n" + "return: " + info );
      throw e;
   }

   try
   {
      for( var i = 0; i < throwCond.length; i++ )
      {
         condFlag = condFlag && errno != throwCond[i];
      }
      if( condFlag ) { throw info; }
   } catch( e )
   {
      println( "tryCatchByHttps message: " + throwOutput );
      println( "command: " + str + "\n" + "return: " + info );
      throw e;
   }
}
/***************************************************************
@description:	to get current function name
@modify list:	
				2015-4-6 Ting YU init 
***************************************************************/
function getFuncName ()
{
   var func = getFuncName.caller.toString();
   var re = /function\s*(\w*)/i;
   var funcName = re.exec( func );
   return funcName[1] + "()";
}
