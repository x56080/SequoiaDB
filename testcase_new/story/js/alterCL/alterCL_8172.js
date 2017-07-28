/******************************************************************************
@Description : 1.alters replsize basic
@Modify list :
               2015-01-22  pusheng Ding  Init
******************************************************************************/

function main()
{
   var clName = COMMCLNAME + "_8172";
	try{
		var db = new Sdb(COORDHOSTNAME, COORDSVCNAME) ;
      db.setSessionAttr( { PreferedInstance: "M" } );
	}catch(e)
	{
		println("can't connect to db");
		throw e;
	}
	
	try{
      commDropCL( db, COMMCSNAME, clName, true, true,
                  "drop cl in the beginning" ) ;
	}catch( e ){}
	
	//create CS
	try{
      var varCS = commCreateCS( db, COMMCSNAME, true, "create CS" );
	}catch(e)
	{
		println("can't create CS:" + COMMCSNAME + " rc="+e);
		throw e;
	}
	println("createCS " + COMMCSNAME + " finished");
	
	//create cl
	try{
		var varCL = varCS.createCL(clName,{ReplSize:1});
		var sn1 = db.snapshot(8,{Name:COMMCSNAME+"."+clName});
		var sourceGroup = sn1.current().toObj()['CataInfo'][0]['GroupName'];
	}catch(e)
	{
		println("can't create CL:" + clName + " rc="+e);
		throw e;
	}
	println("createCL " + clName + " at ReplicaGroup:" + sourceGroup + " finished");

	//alters replsize
	try{
		varCL.alter({ReplSize:3});
		var sn1 = db.snapshot(8,{Name:COMMCSNAME + "." + clName});
		var replsize = sn1.current().toObj()['ReplSize'];
		if(replsize == 3)
		{
			println("alters replsize succ!");
		}
		else
		{
			println("alters replsize fail! ReplSize=" + replsize);
			throw 1;
		}	
	}catch(e)
	{
		if(e != -1)
		{
			println("alters replsize fail! rc="+e);
		}
		throw e;
	}
	println("alters replsize finish!");
	
	//clean test-env
	try{
      commDropCL( db, COMMCSNAME, clName, true, true,
                  "drop cl in the end" ) ;
	}catch(e)
	{
		println("clean test-evn fail! rc="+e);
		throw e;
	}
	println("clean test-evn succ!");
}

// Add inspect standalone run mode
try
{
   var db = new Sdb(COORDHOSTNAME, COORDSVCNAME) ;
   // Inspect the run mode is standalone or not
   if( true == commIsStandalone( db ) )
      throw "ModeStandAlone" ;
   main();
}
catch( e )
{
   if( "ModeStandAlone" == e )
      println( "The run mode is standalone" ) ;
   else
      throw e ;
}

