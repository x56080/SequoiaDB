##语法##

***System.getCpuInfo()***

##类别##

System

##描述##

获取CPU的信息

##参数##

无

##返回值##

返回CPU的信息

##错误##

如果出错则抛异常，并输出错误信息，可以通过[getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](reference/Sequoiadb_command/Global/getLastError.md)获取错误码。
关于错误处理可以参考[常见错误处理指南](manual/faq.md)。

常见错误可参考[错误码](reference/Sequoiadb_error_code.md)。

##示例##

* 获取CPU的信息

	```lang-javascript
  > System.getCpuInfo()
  {
      "Cpus": [
        {
          "Core": 1,
          "Info": "Intel(R) Xeon(R) CPU E5-2650 v4 @ 2.20GHz",
          "Freq": "2.19999814GHz"
        },
        {
          "Core": 1,
          "Info": "Intel(R) Xeon(R) CPU E5-2650 v4 @ 2.20GHz",
          "Freq": "2.19999814GHz"
        }
      ],
      "User": 47223380,
      "Sys": 46662920,
      "Idle": 3513293040,
      "Other": 3023840
  }
	```