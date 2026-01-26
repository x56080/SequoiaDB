import { Message } from 'element-ui'
import router from '@/router'
import defaultSettings from '@/settings'
const { maxMessageSize } = defaultSettings


let messageList = []
const SdbMessage = options => {
  messageList = messageList.filter(item => !item.closed)
  if (messageList.length < maxMessageSize) {
    messageList.push(Message(options))
  } else {
    messageList[0].close()
    messageList.shift()
    messageList.push(Message(options))
  }
}

const messageTypeList = ['error', 'success', 'info', 'warning']
messageTypeList.forEach(type => {
  SdbMessage[type] = options => {
    if (typeof options === 'string') {
      options = {
        message: options
      }
    }
    options.type = type
    return SdbMessage(options)
  }
})

router.afterEach(() => {
  // clear all message
  Message.closeAll()
})

export const message = SdbMessage
