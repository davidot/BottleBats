<template>
  <div class="bot-item" @click="toggleDetails">
    <span :style="{'color': bot.enabled ? 'green' : bot.failed ? 'red' : 'yellow'}">
      {{ bot.name }}
    </span>
    : {{ statusTitle }}
    <div class="bot-details" v-show="detailsShown">
      <code style="white-space: pre" v-if="bot.failed">
        {{ detailedStatus || 'Iets ging mis weet niet wat?' }}
      </code>
      <div v-else style="display: flex; flex-direction: row; height: 40px; text-align: center; vertical-align: center; justify-items: center; justify-content: center">
        Games gespeeld: {{ playedGames }} Games gewonnen: {{ wonGames }}
      </div>
    </div>
  </div>
</template>

<script>
import {endpoint} from "@/http";

export default {
  name: "BotItem",
  props: {
    bot: {botId: Number, status: String, name: String},
  },
  data() {
    return {
      detailsShown: false,
      playedGames: 'loading',
      wonGames: 'loading',
    };
  },
  computed: {
    statusTitle() {
      if (!this.bot || !this.bot.state)
        return 'No status';

      return this.bot.state.split('\n')[0].trim();
    },
    detailedStatus() {
      if (!this.bot || !this.bot.state)
        return '';

      const parts = this.bot.state.split('\n');
      if (parts.length < 2)
        return '';

      parts.shift();

      return parts.join('\n');
    }
  },
  methods: {
    toggleDetails() {
      this.detailsShown = !this.detailsShown;
      this.getStats();
    },
    getStats() {
      if (!this.bot || this.bot.failed)
        return;

      endpoint.get('/vijf/bot/' + this.bot.botId)
          .then((val) => {
            this.playedGames = val.data.played;
            this.wonGames = val.data.won;
          })
          .catch(() => {
            console.log('Failed to get bot stats!');
          });

    }
  },
};
</script>

<style scoped>
.bot-item {
  width: 100%;
  border: 1px solid black;
  border-top-width: 0;
}

.bot-item:first-child {
  border-top-width: 1px;
}

.bot-details {
  border-top: 2px solid black;
  margin-top: 2px;
  margin-bottom: 2px;
}
</style>
