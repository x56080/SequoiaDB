<template>
  <div>
    <div>
      每
      <el-select id="input_cronPicker_perMinute" v-model="perMinute" size="mini" style="width: 65px" @change="emitChange()">
        <el-option
          v-for="item in minuteOptions"
          :key="item"
          :label="item"
          :value="item"
        />
      </el-select>
      分钟运行一次
    </div>
  </div>
</template>

<script>

export default {
  name: 'CronMinute',
  data() {
    return {
      perMinute: 5
    }
  },
  computed: {
    minuteOptions() {
      return [5, 10, 15, 20, 30]
    },
    cronExp() {
      return `0 */${this.perMinute} * * * ?`
    }
  },
  methods: {
    init(value) {
      const tempArr = value.split(' ')
      const minuteArr = tempArr[1].split('/')
      this.perMinute = Number(minuteArr[1])
    },
    emitChange() {
      this.$emit('change', this.cronExp)
    },
    
  }
}
</script>

<style scoped>
.divider{
  color: #949AA6;
}
</style>
