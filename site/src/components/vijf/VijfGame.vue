<template>
  <button @click="shuffle">Shuffle!</button>
  <button @click="moveToTop">To top!</button>
  <button @click="moveToBottom">To bottom!</button>
  <label for="showDeckToggle">Show deck cards</label><input id="showDeckToggle" type="checkbox" v-model="showdeck">
  <label for="playToggle">Playing</label><input id="playToggle" type="checkbox" v-model="replayState.playing">
  {{ textOut }} {{ replayState.cardsToDeal }}
  <div class="vijf-table">
    <div v-for="(player, pIndex) in players" :key="'playerback' + pIndex"
         :style="{
        position: 'absolute',
        top: '70%',
        left: 'calc(' + (15 + pIndex * 15) + '%)',
        width: '150px',
        height: '100px',
        background: 'red',
        opacity: player.length > 0 ? 0 : 1,
        transition: 'opacity 10s ease',
      }">
    </div>
    <transition-group name="card-deck">
      <template v-for="(card, index) in deck" :key="'card' + card.id">
        <playing-card
            :type="card.type"
            :backside="!showdeck"
            class="cards-deck"
            :style="{
              top: '10%', left: 'calc(10% + ' + (deck.length * 25 - index * 25) + 'px)',
              opacity: Math.min(Math.max(0, 1 - ((deck.length - index - 1) - 5)/5), 1),
              visibility: index >= (deck.length - 15) ? 'visible':'hidden'
            }"
        />
      </template>
      <playing-card
        @clicked="cardClicked"
        v-for="(card, index) in discarded"
        :key="'card' + card.id"
        :type="card.type"
        class="cards-discarded"
        :style="{ top: '10%', left: 'calc(35% + ' + index * 25 + 'px)', visibility: 'visible' }"
      />

      <template v-for="(player, pIndex) in players">
        <playing-card
          @clicked="cardClicked"
          v-for="(card, index) in player"
          :key="'card' + card.id"
          :type="card.type"
          class="player-cards"
          :style="{
            top: '70%',
            left: 'calc(' + (15 + pIndex * 15) + '% + ' + index * 25 + 'px)',
            visibility: 'visible',
          }"
        />
      </template>
    </transition-group>
  </div>
</template>

<script>
import PlayingCard from "@/components/vijf/Card.vue";

class CardConverter {
  constructor() {
    this.cardNum = {};
  }

  do(str) {
    return [...str].map((c) => {
      if (!(c in this.cardNum)) {
        this.cardNum[c] = 0;
      }

      let num = this.cardNum[c]++;

      return {
        type: c,
        id: c + "_" + num,
      };
    });
  }
}

function defaultDeck() {
  return new CardConverter().do(
    "**+A23456789TJQKA23456789TJQKA23456789TJQKA23456789TJQK"
  );
}

function isRulesCard(c) {
  return c === '+';
}

const drawMap = {
  '+': -1, // Rules
  '*': 0,
  'A': 1,
  '2': 2,
  '3': 3,
  '4': 4,
  '5': 5,
  '6': 6,
  '7': 7,
  '8': 8,
  '9': 9,
  'T': 10,
  'J': 11,
  'Q': 12,
  'K': 13
};

function cardToDraw(c) {
  return drawMap[c];
}

export default {
  name: "VijfGame",
  mounted() {
    this.timer.lastHit = +new Date();
    this.timer.id = setInterval(() => this.timerHit(), 20);
  },
  unmounted() {
    clearInterval(this.timer.id);
  },
  components: {
    PlayingCard,
  },
  props: {
    start: String,
    moves: String,
  },
  data() {
    const startDeck = defaultDeck();
    return {
      timer: {
        id: null,
        lastHit: null,
        realInterval: 1000,
      },
      discarded: [], //startDeck.splice(0, 5),
      deck: startDeck,
      players: [[], [], [], [], []],
      textOut: "",
      invalid: false,
      showdeck: false,
      replayState: {
        playing: false,
        nextMoveIndex: 0,
        cardsToDeal: -1,
        rulesIndex: -1,
        currentPlayerTurn: 100, // start as invalid to ensure first alive starts
      },
    };
  },
  computed: {
    madeMoves() {
      return this.moves.split(' ')[1] || '';
    }
  },
  methods: {
    timerHit() {
      const now = +new Date();
      if ((now - this.timer.lastHit) < this.timer.realInterval)
        return;

      this.timer.lastHit = now;

      this.progressReplay();
    },
    progressReplay() {
      if (!this.replayState.playing)
        return;

      if (this.replayState.cardsToDeal < 0 && this.replayState.rulesIndex < 0) {
        // Go to next state move
        if (this.replayState.nextMoveIndex >= this.madeMoves.length) {
          console.log('Ran out of moves stopping...');
          this.replayState.playing = false;
          return;
        }

        const newMove = this.madeMoves.charAt(this.replayState.nextMoveIndex++);
        console.log('Making move: ' + newMove);
        /*this.replayState.currentPlayerTurn = */this.advancePlayer();
        if (isRulesCard(newMove)) {
          // Uhhh
          throw "FIXME RULES CARD!!";
        } else {
          this.replayState.cardsToDeal = cardToDraw(newMove) - 1;
          console.log('Now have to draw: ', this.replayState.cardsToDeal, ' + 1 of course');
          const index = this.players[this.replayState.currentPlayerTurn].find(
              (a) => a.type === newMove
          );

          if (index == null)
            throw "INVALID REPLAY?";

          this.discarded.push(
            ...this.players[this.replayState.currentPlayerTurn].splice(index, 1)
          );
        }
        return;
      }

      // FIXME: Rules cards

      if (this.replayState.cardsToDeal >= 0) {
          --this.replayState.cardsToDeal;
          const drawn = this.deck.pop();
          if (drawn.type === '5') {
            this.discarded.push(...this.players[this.replayState.currentPlayerTurn]);
            this.discarded.push(drawn);
            this.players[this.replayState.currentPlayerTurn] = [];
          } else {
            this.players[this.replayState.currentPlayerTurn].push(drawn);
          }
          if (drawn.type === '5' || drawn.type === '+')
            this.replayState.cardsToDeal = -1;

      } else {
        throw "FIXME RULES CARD!";
      }
    },
    advancePlayer() {
      console.log(0, this.replayState.currentPlayerTurn, this.players.length);
      const inc = () =>
        (this.replayState.currentPlayerTurn =
          (this.replayState.currentPlayerTurn + 1) % this.players.length);

      inc();
      console.log(1, this.replayState.currentPlayerTurn, this.players.length);
      while (this.players[this.replayState.currentPlayerTurn].length === 0) {
        inc();
        console.log(2, this.replayState.currentPlayerTurn);
      }

      return this.replayState.currentPlayerTurn;
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
    clearAll() {
      this.deck = [];
      this.discarded = [];
      for (let i = 0; i < this.players.length; ++i) {
        this.players[i] = [];
      }
    },
    reset() {
      const parts = this.start.split(";");
      this.replayState = {
        playing: false,
        nextMoveIndex: 0,
        cardsToDeal: -1,
        rulesIndex: -1,
        currentPlayerTurn: 0,
      };
      if (parts.length !== 7) {
        this.clearAll();
        this.invalid = true;
        return;
      }

      const converter = new CardConverter();

      for (let i = 0; i < 5; ++i) {
        this.players[i] = converter.do(parts[i]);
      }

      this.deck = converter.do(parts[5]);
      this.deck.reverse();
      this.discarded = converter.do(parts[6]);
    },
  },
  watch: {
    start: {
      handler(newValue, oldValue) {
        console.log('new ', newValue, ' old', oldValue)
        if (oldValue !== undefined || newValue !== "")
          this.reset();
      },
      immediate: true,
    },
    moves() {
      this.reset();
      this.nextMoveIndex = 0;
    },
  },
};
</script>

<style scoped>
.vijf-table {
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

.playing-card {
  transition: all .5s ease;
}

.card-deck-move, /* apply transition to moving elements */
.card-deck-enter-active,
.card-deck-leave-active {

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
