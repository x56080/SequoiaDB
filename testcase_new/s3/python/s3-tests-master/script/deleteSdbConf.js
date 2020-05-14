  var sdb;
  if(typeof(hostname) == undefined)
  {
   throw 'hostname must be defined';
  }
  
  if(typeof(port) == undefined)  
  {
   throw 'port must be defined';
  }

  if(typeof(username) == "undefined" && typeof(password) == "undefined")
  {
    sdb = new Sdb(hostname, port);  
  
  }
  else if(typeof(username) != "undefined" && typeof(password) != "undefined")
  {
    sdb = new Sdb(hostname, port,username, psssword);
  }
  else
  {
    throw 'username and password must be defined at the same time'
  }

  if(typeof(config) == "undefined" )
  {
    throw 'config must be defined'
  }
  println("INFO:begin delete sdb conf:" + config)
  sdb.deleteConf(JSON.parse(config))
  println("INFO:end delete sdb conf")
