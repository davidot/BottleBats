<template>
  <div>
    <div style="display: flex; flex-direction: row">
      <div style="display: flex; place-items: center;">
        <span class="name"> Hello!</span> {{ count }}
        <button @click="addMe">Click</button>
        <button @click="removeMe">Remove</button>
        <button @click="shuffle">Shuffle</button>
      </div>
      <div style="height: 500px; width: 500px; overflow-y: scroll; overflow-x: hidden">
        <div style="width: 80%; margin: auto; display: flex; flex-direction: column">
          <TransitionGroup name="alist" move-class="moving-item">
            <component :is="'vijf-leaderboard-item'" v-for="(item, index) in orderedItems" :key="'lb-' + item.itemId" :data="item" class="anim-item" :data-change="index <= (oldIndices[index] || -1) ? 'positive-change' : 'negative-change'">
              {{ index }} vs {{ oldIndices[index] || -1 }}
            </component>
          </TransitionGroup>
        </div>
      </div>
    </div>
  </div>
</template>

<script>

import VijfLeaderboardItem from "@/components/vijf/VijfLeaderboardItem.vue";

export default {
  name: "LeaderBoard",
  components: {
    VijfLeaderboardItem,
  },
  data() {
    return {
      count: 1,
      things: [{ rank: 2, itemId: '123y5d', name: "hi" }, {rank: 0, itemId: '123152by5d',name: "hello"}, {rank: 1, itemId: 'asdg80021',name: "bye"}],
      oldIndices: [],
    };
  },
  computed: {
    orderedItems() {
      return [...this.things].sort((lhs, rhs) => lhs.rank - rhs.rank);
    },
  },
  methods: {
    fillOldIndices() {
      this.oldIndices = [...this.things.keys()];
    },
    addMe() {
      // this.things.push({nm: this.things[this.things.length - 1].nm + 'b'});

      this.fillOldIndices();
    },
    removeMe() {
      this.fillOldIndices();

      const index = Math.random() * this.things.length;
      this.things.splice(index, 1);
    },
    shuffle() {
      this.fillOldIndices();

      const indices = [...this.things.keys()];

      for (let i = 0; i < this.things.length; i++) {
        let number = Math.floor(i + (this.things.length - i) * Math.random());
        const temp = indices[i];
        indices[i] = indices[number];
        indices[number] = temp;
      }

      for (let i = 0; i < this.things.length; i++) {
        // const tempIdx = this.oldIndices[i];
        // this.oldIndices[i] = this.oldIndices[number];
        // this.oldIndices[number] = tempIdx;

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
