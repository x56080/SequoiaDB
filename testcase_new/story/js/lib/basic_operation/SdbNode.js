var tmpSdbNode = {
   connect: SdbNode.prototype.connect,
   getHostName: SdbNode.prototype.getHostName,
   getNodeDetail: SdbNode.prototype.getNodeDetail,
   getServiceName: SdbNode.prototype.getServiceName,
   help: SdbNode.prototype.help,
   start: SdbNode.prototype.start,
   stop: SdbNode.prototype.stop,
   toString: SdbNode.prototype.toString
};
var funcSdbNode = SdbNode;
var funcSdbNodehelp = SdbNode.help;
SdbNode=function(){try{return funcSdbNode.apply( this, arguments ); } catch( e ) { var msg = e.message || e; throw new Error(msg) } };
SdbNode.help = function(){try{ return funcSdbNodehelp.apply( this, arguments ); } catch( e ) { var msg = e.message || e; throw new Error(msg) } };
SdbNode.prototype.connect=function(){try{return tmpSdbNode.connect.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
SdbNode.prototype.getHostName=function(){try{return tmpSdbNode.getHostName.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
SdbNode.prototype.getNodeDetail=function(){try{return tmpSdbNode.getNodeDetail.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
SdbNode.prototype.getServiceName=function(){try{return tmpSdbNode.getServiceName.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
SdbNode.prototype.help=function(){try{return tmpSdbNode.help.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
SdbNode.prototype.start=function(){try{return tmpSdbNode.start.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
SdbNode.prototype.stop=function(){try{return tmpSdbNode.stop.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
SdbNode.prototype.toString=function(){try{return tmpSdbNode.toString.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
