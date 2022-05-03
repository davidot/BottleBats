<template>
  <div class="elevated-bot-info" @click="toggle">
    <span :style="{'color': bot.running ? 'green' : 'red', 'border-color': bot.running ? 'green' : 'red'}">
      {{ bot.name }} ({{ bot.id }})
    </span>
    : {{ bot.status }}
    <button style="float: right" @click.stop="showDetails">
      {{ detailsShown ? 'Refresh' : 'Details'}}
    </button>
    <div class="bot-details" v-show="detailsShown">
      <hr/>
      <div v-if="!cases">
        Loading...
      </div>
      <div style="display: flex; flex-direction: row; flex-wrap: wrap">
        <div v-for="cs in cases" :key="'case-' + cs.id + '-for-' + bot.id" class="bot-case" :style="{'border-color': cs.success ? 'green' : 'red'}">
          <h4 style="text-align: center; margin: 2px">
            {{cs.name}}
          </h4>
          <hr />
          <div v-if="cs.running">
            <spinner /> Running case
          </div>
          <div v-else-if="cs.success">
            Passed! with stats:
            <ul>
              <li v-for="stat in Object.entries(cs.result)" :key="cs.id + '-' + bot.id + '-stat-' + stat[0]">
                {{ stat[0] }}: {{ Math.round(stat[1] * 100.0) / 100.0 }}
              </li>
            </ul>
          </div>
          <div v-else>
            Failed :( <br />
            <code style="white-space: pre">
              {{ cs.result || 'Iets ging mis weet niet wat?' }}
            </code>
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<script>
import Spinner from "../Spinner.vue";
import {endpoint} from "@/http";
export default {
  name: "BotInfo",
  props: {
    bot: {id: Number, name: String, running: Boolean, status: String}
  },
  data() {
    return {
      detailsShown: false,
      cases: null,
    };
  },
  methods: {
    toggle() {
      this.detailsShown = !this.detailsShown;
      this.getStats();
    },
    showDetails() {
      this.detailsShown = true;
      this.getStats();
    },
    getStats() {
      if (!this.bot || this.bot.id == null)
        return;

      endpoint.get('/elevated/bot-cases/' + this.bot.id)
        .then((val) => {
          this.cases = val.data;
        })
        .catch(() => {
          console.log('Failed to get bot stats!');
        });

    }
  },
  components: {Spinner},
};
</script>

<style scoped>
.elevated-bot-info {
  width: 60%;
  border: 2px solid;
  border-radius: 5px;
  margin-top: 5px;
  padding: 2px;
}

.elevated-bot-info:first-child {
  border-top: 2px solid;
}

.bot-case {
  border: 1px solid gray;
  border-radius: 5px;
  padding: 2px;
}

</style>
