<template>
  <div>
    <div style="">
      <div style="">
        <div style="width: 80%; margin: auto; display: flex; flex-direction: column">
          <div v-if="connectionLost" style="color: red; position: relative; top: 0; width: 100%; text-align: center;">
            Connection to server lost!
          </div>
          <div v-if="items == null">
            Loading
          </div>
          <div v-else-if="items.length === 0">
            No data
          </div>
          <TransitionGroup v-else name="alist" move-class="moving-item">
            <component :is="'vijf-leaderboard-item'" v-for="(item, index) in orderedItems"
                       :key="'lb-' + item.itemId"
                       :data="item" class="anim-item"
                       :data-change="index <= (oldRanks[item.itemId] || -1) ? 'positive-change' : 'negative-change'">
<!--               / {{ index }} vs {{ oldRanks[item.itemId] || -1 }}-->
            </component>
          </TransitionGroup>
        </div>
      </div>
    </div>
  </div>
</template>

<script>

import VijfLeaderboardItem from "@/components/vijf/VijfLeaderboardItem.vue";
import {endpoint} from "@/http";

export default {
  name: "LeaderBoard",
  components: {
    VijfLeaderboardItem,
  },
  mounted() {
    this.dataInterval = setInterval(() => this.getData(), 1000);
    this.getData();
  },
  unmounted() {
    if (this.dataInterval)
      clearInterval(this.dataInterval);
  },
  data() {
    return {
      dataInterval: null,
      connectionLost: false,
      items: null,
      oldRanks: {},
    };
  },
  computed: {
    orderedItems() {
      return [...this.items].sort((lhs, rhs) => lhs.rank - rhs.rank);
    },
  },
  methods: {
    saveOldRanks() {
      this.oldRanks = {};
      if (this.items == null)
        return;

      for (let item of this.items)
        this.oldRanks[item.itemId] = item.rank;
    },
    async getData() {
      this.saveOldRanks();

      try {
        const data = await endpoint.get("/vijf/leaderboard", {timeout: 750});
        this.items = data.data;
        this.connectionLost = false;
      } catch {
        this.connectionLost = true;
      }
    },
  },
};
</script>

<style scoped>

.anim-item {
  /*display: inline-block;*/

  transition: transform 1s ease,
    color 1s cubic-bezier(0.175, 0.885, 0.32, 1.275);
  border: 1px solid black;
}

.alist-move, /* apply transition to moving elements */
.alist-enter-active,
.alist-leave-active {
}

.alist-enter-from,
.alist-leave-to {
  opacity: 0;
  /*transform: translateX(30px);*/
}

/* ensure leaving items are taken out of layout flow so that moving
   animations can be calculated correctly. */
.alist-leave-active {
  position: absolute;
}

.moving-item {
  /*transition: all 2s ease;*/
  color: green;
  /*transform: translateY(50px);*/
}

.moving-item[data-change="negative-change"] {
  color: red;
}

/*.anim-item:hover .name {*/
/*  transform: scale(120%);*/
/*}*/

.name:hover {
  /*display: inline-block;*/
  transform: scale(150%);
}

/*.anim-item:first-child {*/
/*  transform: scale(125%) translateY(25%);*/
/*  transform-origin: 0 0;*/
/*  margin-bottom: 10px;*/
/*}*/

</style>
