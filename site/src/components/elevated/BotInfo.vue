<template>
  <div class="elevated-bot-info" @click="toggle" :style="{ 'border-color': statusColor }">
    <img v-if="bot.hasImage" :src="'/api/elevated/bot-image/' + bot.id" style="width: 31px; height: 31px; padding-right: 15px" alt=""
         @click.prevent.stop="$emit('image-copy', $event)" loading="lazy"/>
    <span :style="{ color: statusColor, background: 'transparent' }" :class="[building && 'open-doors', building && 'looping-doors']">
      {{ bot.name }} ({{ bot.id }})
    </span>
    : {{ bot.status }} {{ workString }}
    <button style="float: right" @click.stop="showDetails">
      {{ detailsShown ? 'Refresh' : 'Details'}}
    </button>
    <button style="float: right" v-if="bot.running" @click.stop="removeBot">
      Remove from leaderboard
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
    bot: {
      id: Number,
      name: String,
      running: Boolean | null,
      status: String,
      hasImage: Boolean,
    },
  },
  data() {
    return {
      detailsShown: false,
      cases: null,
    };
  },
  mounted() {
    if (this.bot.running !== false)
      this.getStats();
  },
  computed: {
    building() {
      if (!this.bot || !this.bot.id || this.bot.running === false)
        return false;
      if (this.bot.running === null)
        return true;
      if (!this.cases)
        return false;

      if (this.cases.length === 0)
        return true;

      return this.cases.some(c => c.running);
    },
    statusColor() {
      if (this.bot.running === true)
        return "#1c6f1d";
      if (this.bot.running === null)
        return "#cc8f1b";
      if (this.bot.status.includes("Disabled by user"))
        return "#576767";
      return "#bb1b1b";
    },
    workString() {
      if (!this.cases)
        return '';
      const total = this.cases.length;
      const done = this.cases.filter(c => c.running === false).length;
      return "(" + done + " / " + total + ")";
    }
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
      if (this.bot?.id == null)
        return;

      endpoint.get('/elevated/bot-cases/' + this.bot.id)
        .then((val) => {
          this.cases = val.data;
        })
        .catch(() => {
          console.log('Failed to get bot stats!');
        });

      setTimeout(() => this.updateIfBuilding(), 1000);
    },
    updateIfBuilding() {
      if (this.$route.name !== 'bots')
        return;

      if (this.building || !this.cases || this.cases.length === 0) {
        this.getStats();
        this.$emit('build-update');
      }
    },
    removeBot() {
      endpoint.get('/elevated/remove-bot/' + this.bot.id)
          .then(() => {
            this.$emit('build-update');
          })
          .catch(() => {
            console.log('Failed to remove bot!');
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
  overflow: hidden;
}

.elevated-bot-info:first-child {
  border-top: 2px solid;
}

.bot-case {
  border: 1px solid gray;
  border-radius: 5px;
  padding: 2px;
}

.looping-doors {
  visibility: hidden;
  animation: after-doors step-end 3s forwards infinite;
}

.looping-doors::before {
  visibility: visible !important;
  animation-name: loading-doors;
  animation-iteration-count: infinite;
  animation-duration: 3s;
}

.looping-doors::after {
  visibility: visible !important;
  animation-name: loading-doors;
  animation-iteration-count: infinite;
  animation-duration: 3s;
}


@keyframes after-doors {
  20% {
    visibility: hidden;
  }
  40% {
    visibility: visible;
  }
  80% {
    visibility: hidden;
  }
}

@keyframes loading-doors {
  0% {
    top: 100%;
    width: 50%;
  }

  20% {
    top: 0;
    width: 50%;
  }

  40% {
    top: 0;
    width: 50%;
  }

  55% {
    top: 0;
    width: 0;
  }

  65% {
    top: 0;
    width: 0;
  }

  80% {
    top: 0;
    width: 50%;
  }

  100% {
    top: -200%;
    width: 50%;
  }

}
</style>
