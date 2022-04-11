<template>
  <div>
    <p>Bots</p>
    <div v-if="!loggedIn">
      Sorry, your not logged in so not much to see here.
    </div>
    <BotCreator v-else @new-bot="updateBots"/>
    <ul>
      <li v-if="bots === null"><div style="animation: 1s linear infinite; transform-origin: 50% 50%;">Loading</div></li>
      <li v-for="(bot, index) in bots" :key="'bot' + index">
        {{ bot.name }} : {{ bot.state }}
      </li>
    </ul>
  </div>
</template>

<script>
import { endpoint } from "@/http";
import BotCreator from "@/components/BotCreator.vue";
import axios from "axios";

export default {
  name: "BotView",
  components: {BotCreator},
  inject: ["userDetails"],
  mounted() {
    this.botUpdateInterval = setInterval(() => this.updateBots(), 15000);
    this.updateBots();
  },
  unmounted() {
    if (this.botUpdateInterval)
      clearInterval(this.botUpdateInterval);
  },
  data() {
    return {
      botUpdateInterval: null,
      bots: null,
      connectionLost: false,
    };
  },
  computed: {
    loggedIn() {
      return this.userDetails.values.value.displayName != null;
    },
  },
  methods: {
    async updateBots() {
      try {
        const data = await endpoint.get("/vijf/bots", {timeout: 750});
        this.bots = data.data;
        this.connectionLost = false;
      } catch {
        this.connectionLost = true;
      }
    },
  },
};
</script>

<style>
@keyframes spinning {
  from {
      transform: rotate(0deg);
  }

  to {
    transform: rotate(360deg);
  }
}

</style>
