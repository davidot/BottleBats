<template>
  <div>
    <p>Bots</p>
    <div v-if="!loggedIn">
      Sorry, your not logged in so not much to see here.
    </div>
    <BotCreator v-else @new-bot="updateBots"/>
    <div style="display: flex; flex-direction: column;">
      <div v-if="bots === null"><div style="animation: 1s linear infinite; transform-origin: 50% 50%;">Loading</div></div>
      <TransitionGroup v-else name="bot-list">
        <BotItem :bot="bot" v-for="bot in bots" :key="'bot' + bot.name" />
      </TransitionGroup>
    </div>
  </div>
</template>

<script>
import { endpoint } from "@/http";
import BotCreator from "@/components/BotCreator.vue";
import BotItem from "@/components/BotItem.vue";

export default {
  name: "BotView",
  components: { BotCreator, BotItem },
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

.bot-list-move, /* apply transition to moving elements */
.bot-list-enter-active,
.bot-list-leave-active {
}

.bot-list-enter-from,
.bot-list-leave-to {
  opacity: 0;
  /*transform: translateX(30px);*/
}

.bot-list-leave-active {
  position: absolute;
}

</style>
