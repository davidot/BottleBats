<template>
  <button @click="shuffle">Shuffle!</button>
  <button @click="moveToTop">To top!</button>
  <button @click="moveToBottom">To bottom!</button>
  {{ textOut }}
  <div class="deck">
    <transition-group name="card-deck">
      <playing-card
        @clicked="cardClicked"
        v-for="(n, index) in deck"
        :key="'card' + n"
        :type="'' + n"
        backside
        class="cards-deck"
        :style="{ top: '10%', left: 'calc(10% + ' + index * 26 + 'px)' }"
      />
      <playing-card
        @clicked="cardClicked"
        v-for="(n, index) in discarded"
        :key="'card' + n"
        :type="'' + n"
        class="cards-discarded"
        :style="{ top: '35%', left: 'calc(10% + ' + index * 25 + 'px)' }"
      />

      <template v-for="(player, pIndex) in players">
        <playing-card
            @clicked="cardClicked"
            v-for="(n, index) in player"
            :key="'card' + n"
            :type="'' + n"
            class="player-cards"
            :style="{ top: '70%', left: 'calc(' + (15 + pIndex * 15) + '% + ' + index * 25 + 'px)' }"
        />
      </template>
    </transition-group>
  </div>
</template>

<script>
import PlayingCard from "@/components/games/Card.vue";
export default {
  name: "VijfGame",
  components: {
    PlayingCard,
  },
  data() {
    return {
      deck: ["+", "*", "A", "K",],
      discarded: ["2"],
      players: [
          ["Q", "J", "T", ],
          [],
          ["9", "8", "7", ],
          ["6", "5", ],
          ["4", "3", ],
      ],
      textOut: "",
    };
  },
  methods: {
    shuffle() {
      for (let i = 0; i < this.nums.length; i++) {
        let number = Math.floor(i + (this.nums.length - i) * Math.random());
        const temp = this.nums[i];
        this.nums[i] = this.nums[number];
        this.nums[number] = temp;
      }
    },
    moveToTop() {
      if (this.deck.length === 0) return;
      const el = this.deck.pop();
      this.players[0].push(el);
    },
    moveToBottom() {
      if (this.players[0].length === 0) return;
      const el = this.players[0].pop();
      this.deck.push(el);
    },
    cardClicked(num) {
      this.textOut += num;
    },
  },
};
</script>

<style scoped>
.deck {
  /*margin-left: 30px;*/
  /*padding-top: 30px;*/
  /*position: initial;*/
  /*top: 0;*/
  background: #336846;
  border: 25px solid #4e3327;

  width: calc(100% - 50px);
  min-width: 600px;
  height: 500px;
  position: relative;
  border-radius: 150px;
  /*display: flex;*/
  /*flex-direction: column;*/
}

.card-deck-move, /* apply transition to moving elements */
.card-deck-enter-active,
.card-deck-leave-active {
  transition: all 1s ease;
}

.card-deck-enter-from,
.card-deck-leave-to {
  opacity: 0;
  transform: translateX(30px);
}

/* ensure leaving items are taken out of layout flow so that moving
   animations can be calculated correctly. */
.card-deck-leave-active {
  position: absolute;
}

.first {
  top: 0;
}
.second {
  top: 100px;
}

.cards-deck {
  position: absolute;
}

.cards-discarded {
  position: absolute;
}

.player-cards {
  position: absolute;
}

/*.deck .playing-card {*/
/*  position: absolute;*/
/*}*/

</style>
