import axios from 'axios'
import { message as Message } from '@/utils/sdb-message'

// 创建 axios 实例
const service = axios.create({
   baseURL: process.env.VUE_APP_BASE_API,
  // baseURL: 'http://192.168.31.15:9001',
  timeout: 1800000, // 30分钟
  headers: {
    'Content-Type': 'application/json'
  }
})

// 请求拦截器
service.interceptors.request.use(
  config => {
    // 如果传入了 `config.isFormData` 为 true，则使用表单格式
    if (config.isFormData && config.data) {
      const params = new URLSearchParams()
      Object.keys(config.data).forEach(key => params.append(key, config.data[key]))
      config.data = params
      config.headers['Content-Type'] = 'application/x-www-form-urlencoded'
    }
    return config
  },
  error => {
    console.error('Request error:', error)
    return Promise.reject(error)
  }
)

// 响应拦截器
service.interceptors.response.use(
  response => response,
  error => {
    let message = 'Unknown error'
    if (error.response) {
      message = error.response.data?.message || error.response.statusText || message
    } else {
      message = 'Cannot connect to server'
    }

    console.error('Response error:', error)

    if (!error.__CANCEL__ && !error.config?.noAlert) {
      Message({
        message: message.replace(/\n/g, '<br>'),
        type: 'error',
        showClose: true,
        duration: 0,
        dangerouslyUseHTMLString: true
      })
    }

    return Promise.reject(error)
  }
)

// 修改默认超时时间
export const updateAxiosTimeout = (newTimeout) => {
  service.defaults.timeout = newTimeout
}

export const getAxiosTimeout = () => service.defaults.timeout

export default service
