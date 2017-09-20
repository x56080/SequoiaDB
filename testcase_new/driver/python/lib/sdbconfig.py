import json
import os
import os.path

class SdbConfig(object):
   def __init__(self):
      self.get_config()

   def get_config(self):
      config_file = os.path.join(os.getcwd(), "config.json")
      fp = open(config_file)
      configs = json.load(fp)
      self.host_name = configs['HOSTNAME']
      self.service = configs['SVCNAME']
      self.changed_prefix = configs['CHANGEDPREFIX']
      self.rsrv_port_begin = configs['RSRVPORTBEGIN']
      self.rsrv_port_end = configs['RSRVPORTEND']
      self.rsrv_node_dir = configs['RSRVNODEDIR']
      self.work_dir = configs['WORKDIR']
      self.break_on_failure = configs['BREAK_ON_FAILURE']

      fp.close()

sdb_config = SdbConfig()

