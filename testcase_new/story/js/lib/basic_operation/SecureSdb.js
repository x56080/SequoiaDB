var tmpSecureSdb = {
   _resolveCS: SecureSdb.prototype._resolveCS,
   analyze: SecureSdb.prototype.analyze,
   backup: SecureSdb.prototype.backup,
   backupOffline: SecureSdb.prototype.backupOffline,
   cancelTask: SecureSdb.prototype.cancelTask,
   close: SecureSdb.prototype.close,
   createCS: SecureSdb.prototype.createCS,
   createCataRG: SecureSdb.prototype.createCataRG,
   createCoordRG: SecureSdb.prototype.createCoordRG,
   createDomain: SecureSdb.prototype.createDomain,
   createProcedure: SecureSdb.prototype.createProcedure,
   createRG: SecureSdb.prototype.createRG,
   createSpareRG: SecureSdb.prototype.createSpareRG,
   createUsr: SecureSdb.prototype.createUsr,
   deleteConf: SecureSdb.prototype.deleteConf,
   dropCS: SecureSdb.prototype.dropCS,
   dropDomain: SecureSdb.prototype.dropDomain,
   dropUsr: SecureSdb.prototype.dropUsr,
   eval: SecureSdb.prototype.eval,
   exec: SecureSdb.prototype.exec,
   execUpdate: SecureSdb.prototype.execUpdate,
   flushConfigure: SecureSdb.prototype.flushConfigure,
   forceSession: SecureSdb.prototype.forceSession,
   forceStepUp: SecureSdb.prototype.forceStepUp,
   getCS: SecureSdb.prototype.getCS,
   getCataRG: SecureSdb.prototype.getCataRG,
   getCatalogRG: SecureSdb.prototype.getCatalogRG,
   getCoordRG: SecureSdb.prototype.getCoordRG,
   getDC: SecureSdb.prototype.getDC,
   getDomain: SecureSdb.prototype.getDomain,
   getRG: SecureSdb.prototype.getRG,
   getSessionAttr: SecureSdb.prototype.getSessionAttr,
   getSpareRG: SecureSdb.prototype.getSpareRG,
   help: SecureSdb.prototype.help,
   invalidateCache: SecureSdb.prototype.invalidateCache,
   list: SecureSdb.prototype.list,
   listBackup: SecureSdb.prototype.listBackup,
   listCollectionSpaces: SecureSdb.prototype.listCollectionSpaces,
   listCollections: SecureSdb.prototype.listCollections,
   listDomains: SecureSdb.prototype.listDomains,
   listProcedures: SecureSdb.prototype.listProcedures,
   listReplicaGroups: SecureSdb.prototype.listReplicaGroups,
   listSequences: SecureSdb.prototype.listSequences,
   listTasks: SecureSdb.prototype.listTasks,
   loadCS: SecureSdb.prototype.loadCS,
   msg: SecureSdb.prototype.msg,
   reloadConf: SecureSdb.prototype.reloadConf,
   removeBackup: SecureSdb.prototype.removeBackup,
   removeCataRG: SecureSdb.prototype.removeCataRG,
   removeCatalogRG: SecureSdb.prototype.removeCatalogRG,
   removeCoordRG: SecureSdb.prototype.removeCoordRG,
   removeProcedure: SecureSdb.prototype.removeProcedure,
   removeRG: SecureSdb.prototype.removeRG,
   removeSpareRG: SecureSdb.prototype.removeSpareRG,
   renameCS: SecureSdb.prototype.renameCS,
   resetSnapshot: SecureSdb.prototype.resetSnapshot,
   setPDLevel: SecureSdb.prototype.setPDLevel,
   setSessionAttr: SecureSdb.prototype.setSessionAttr,
   snapshot: SecureSdb.prototype.snapshot,
   startRG: SecureSdb.prototype.startRG,
   stopRG: SecureSdb.prototype.stopRG,
   sync: SecureSdb.prototype.sync,
   toString: SecureSdb.prototype.toString,
   traceOff: SecureSdb.prototype.traceOff,
   traceOn: SecureSdb.prototype.traceOn,
   traceResume: SecureSdb.prototype.traceResume,
   traceStatus: SecureSdb.prototype.traceStatus,
   transBegin: SecureSdb.prototype.transBegin,
   transCommit: SecureSdb.prototype.transCommit,
   transRollback: SecureSdb.prototype.transRollback,
   unloadCS: SecureSdb.prototype.unloadCS,
   updateConf: SecureSdb.prototype.updateConf,
   waitTasks: SecureSdb.prototype.waitTasks
};
var funcSecureSdb = SecureSdb;
var funcSecureSdbhelp = SecureSdb.help;
SecureSdb=function(){try{return funcSecureSdb.apply( this, arguments ); } catch( e ) { var msg = e.message || e; throw new Error(msg) } };
SecureSdb.help = function(){try{ return funcSecureSdbhelp.apply( this, arguments ); } catch( e ) { var msg = e.message || e; throw new Error(msg) } };
SecureSdb.prototype._resolveCS=function(){try{return tmpSecureSdb._resolveCS.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
SecureSdb.prototype.analyze=function(){try{return tmpSecureSdb.analyze.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
SecureSdb.prototype.backup=function(){try{return tmpSecureSdb.backup.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
SecureSdb.prototype.backupOffline=function(){try{return tmpSecureSdb.backupOffline.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
SecureSdb.prototype.cancelTask=function(){try{return tmpSecureSdb.cancelTask.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
SecureSdb.prototype.close=function(){try{return tmpSecureSdb.close.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
SecureSdb.prototype.createCS=function(){try{return tmpSecureSdb.createCS.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
SecureSdb.prototype.createCataRG=function(){try{return tmpSecureSdb.createCataRG.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
SecureSdb.prototype.createCoordRG=function(){try{return tmpSecureSdb.createCoordRG.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
SecureSdb.prototype.createDomain=function(){try{return tmpSecureSdb.createDomain.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
SecureSdb.prototype.createProcedure=function(){try{return tmpSecureSdb.createProcedure.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
SecureSdb.prototype.createRG=function(){try{return tmpSecureSdb.createRG.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
SecureSdb.prototype.createSpareRG=function(){try{return tmpSecureSdb.createSpareRG.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
SecureSdb.prototype.createUsr=function(){try{return tmpSecureSdb.createUsr.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
SecureSdb.prototype.deleteConf=function(){try{return tmpSecureSdb.deleteConf.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
SecureSdb.prototype.dropCS=function(){try{return tmpSecureSdb.dropCS.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
SecureSdb.prototype.dropDomain=function(){try{return tmpSecureSdb.dropDomain.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
SecureSdb.prototype.dropUsr=function(){try{return tmpSecureSdb.dropUsr.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
SecureSdb.prototype.eval=function(){try{return tmpSecureSdb.eval.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
SecureSdb.prototype.exec=function(){try{return tmpSecureSdb.exec.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
SecureSdb.prototype.execUpdate=function(){try{return tmpSecureSdb.execUpdate.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
SecureSdb.prototype.flushConfigure=function(){try{return tmpSecureSdb.flushConfigure.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
SecureSdb.prototype.forceSession=function(){try{return tmpSecureSdb.forceSession.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
SecureSdb.prototype.forceStepUp=function(){try{return tmpSecureSdb.forceStepUp.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
SecureSdb.prototype.getCS=function(){try{return tmpSecureSdb.getCS.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
SecureSdb.prototype.getCataRG=function(){try{return tmpSecureSdb.getCataRG.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
SecureSdb.prototype.getCatalogRG=function(){try{return tmpSecureSdb.getCatalogRG.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
SecureSdb.prototype.getCoordRG=function(){try{return tmpSecureSdb.getCoordRG.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
SecureSdb.prototype.getDC=function(){try{return tmpSecureSdb.getDC.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
SecureSdb.prototype.getDomain=function(){try{return tmpSecureSdb.getDomain.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
SecureSdb.prototype.getRG=function(){try{return tmpSecureSdb.getRG.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
SecureSdb.prototype.getSessionAttr=function(){try{return tmpSecureSdb.getSessionAttr.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
SecureSdb.prototype.getSpareRG=function(){try{return tmpSecureSdb.getSpareRG.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
SecureSdb.prototype.help=function(){try{return tmpSecureSdb.help.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
SecureSdb.prototype.invalidateCache=function(){try{return tmpSecureSdb.invalidateCache.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
SecureSdb.prototype.list=function(){try{return tmpSecureSdb.list.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
SecureSdb.prototype.listBackup=function(){try{return tmpSecureSdb.listBackup.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
SecureSdb.prototype.listCollectionSpaces=function(){try{return tmpSecureSdb.listCollectionSpaces.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
SecureSdb.prototype.listCollections=function(){try{return tmpSecureSdb.listCollections.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
SecureSdb.prototype.listDomains=function(){try{return tmpSecureSdb.listDomains.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
SecureSdb.prototype.listProcedures=function(){try{return tmpSecureSdb.listProcedures.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
SecureSdb.prototype.listReplicaGroups=function(){try{return tmpSecureSdb.listReplicaGroups.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
SecureSdb.prototype.listSequences=function(){try{return tmpSecureSdb.listSequences.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
SecureSdb.prototype.listTasks=function(){try{return tmpSecureSdb.listTasks.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
SecureSdb.prototype.loadCS=function(){try{return tmpSecureSdb.loadCS.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
SecureSdb.prototype.msg=function(){try{return tmpSecureSdb.msg.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
SecureSdb.prototype.reloadConf=function(){try{return tmpSecureSdb.reloadConf.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
SecureSdb.prototype.removeBackup=function(){try{return tmpSecureSdb.removeBackup.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
SecureSdb.prototype.removeCataRG=function(){try{return tmpSecureSdb.removeCataRG.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
SecureSdb.prototype.removeCatalogRG=function(){try{return tmpSecureSdb.removeCatalogRG.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
SecureSdb.prototype.removeCoordRG=function(){try{return tmpSecureSdb.removeCoordRG.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
SecureSdb.prototype.removeProcedure=function(){try{return tmpSecureSdb.removeProcedure.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
SecureSdb.prototype.removeRG=function(){try{return tmpSecureSdb.removeRG.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
SecureSdb.prototype.removeSpareRG=function(){try{return tmpSecureSdb.removeSpareRG.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
SecureSdb.prototype.renameCS=function(){try{return tmpSecureSdb.renameCS.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
SecureSdb.prototype.resetSnapshot=function(){try{return tmpSecureSdb.resetSnapshot.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
SecureSdb.prototype.setPDLevel=function(){try{return tmpSecureSdb.setPDLevel.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
SecureSdb.prototype.setSessionAttr=function(){try{return tmpSecureSdb.setSessionAttr.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
SecureSdb.prototype.snapshot=function(){try{return tmpSecureSdb.snapshot.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
SecureSdb.prototype.startRG=function(){try{return tmpSecureSdb.startRG.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
SecureSdb.prototype.stopRG=function(){try{return tmpSecureSdb.stopRG.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
SecureSdb.prototype.sync=function(){try{return tmpSecureSdb.sync.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
SecureSdb.prototype.toString=function(){try{return tmpSecureSdb.toString.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
SecureSdb.prototype.traceOff=function(){try{return tmpSecureSdb.traceOff.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
SecureSdb.prototype.traceOn=function(){try{return tmpSecureSdb.traceOn.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
SecureSdb.prototype.traceResume=function(){try{return tmpSecureSdb.traceResume.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
SecureSdb.prototype.traceStatus=function(){try{return tmpSecureSdb.traceStatus.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
SecureSdb.prototype.transBegin=function(){try{return tmpSecureSdb.transBegin.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
SecureSdb.prototype.transCommit=function(){try{return tmpSecureSdb.transCommit.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
SecureSdb.prototype.transRollback=function(){try{return tmpSecureSdb.transRollback.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
SecureSdb.prototype.unloadCS=function(){try{return tmpSecureSdb.unloadCS.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
SecureSdb.prototype.updateConf=function(){try{return tmpSecureSdb.updateConf.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
SecureSdb.prototype.waitTasks=function(){try{return tmpSecureSdb.waitTasks.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
