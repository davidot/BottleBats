<template>
  <div :class="['playing-card', colorClass, backside ? 'backside' : 'frontside']" @click="clicked">
    <span class="card-suit suit-top">{{ suitText }}</span>
    <span class="card-type">{{ text }}</span>
    <span class="card-suit suit-bottom">{{ suitText }}</span>
  </div>
</template>

<script>
const cardMap = {
  T: {
    type: "10",
  },
  "*": {
    type: "Joker",
    special: "blue",
  },
  "+": {
    type: "Rules",
    special: "blue",
  },
  5: {
    type: "5",
    special: "red",
  },
};

export default {
  name: "Playing-Card",
  props: {
    type: String,
    backside: {
      type: Boolean,
      default: false,
    },
  },
  computed: {
    text() {
      return cardMap[this.type]?.type || this.type;
    },
    suitText() {
      return this.text.charAt(0).toUpperCase();
    },
    colorClass() {
      return cardMap[this.type]?.special || "";
    },
  },
  methods: {
    clicked() {
      this.$emit("clicked", this.type);
    },
  },
};
</script>

<style scoped>
.playing-card {
  width: 75px;
  height: 100px;
  float: left;
  margin-right: 5px;
  margin-bottom: 5px;
  border-radius: 2px;

  position: relative;
  max-width: 100%;
  background-color: #fff;
  box-shadow: -5px 2px 3px rgba(10, 10, 10, 0.1), 0 0 0 1px rgba(10, 10, 10, 0.1);
  color: #4a4a4a;

  /*margin-left: -50px;*/
}

.playing-card:first-child {
  margin-left: 0;
}

.card-suit {
  transition: visibility 5s ease;

  width: 100%;
  display: block;
}

.suit-top {
  text-align: left;
  padding-left: 5px;
}

.suit-bottom {
  position: absolute;
  bottom: 5px;
  text-align: left;
  transform: rotate(180deg);
  margin-left: -5px;
}

.card-type {
  width: 100%;
  position: absolute;
  top: 38%;
  text-align: center;
  font-size: 125%;
}

.backside .card-type,
.backside .card-suit {
  visibility: hidden;
}

.backside:nth-child(odd) {
  background-color: #e5e5f7;
  background: repeating-linear-gradient( 45deg, #444cf7, #444cf7 6px, #e5e5f7 6px, #e5e5f7 20px );
}

.backside:nth-child(even) {
  /*background-color: #e5e5f7;*/
  /*background: linear-gradient(135deg, #444cf755 25%, transparent 25%) -35px 0/ 70px 70px, linear-gradient(225deg, #444cf7 25%, transparent 25%) -35px 0/ 70px 70px, linear-gradient(315deg, #444cf755 25%, transparent 25%) 0px 0/ 70px 70px, linear-gradient(45deg, #444cf7 25%, #e5e5f7 25%) 0px 0/ 70px 70px;*/

  background-color: #e5e5f7;
  background: repeating-linear-gradient( -45deg, #444cf7, #444cf7 6px, #e5e5f7 6px, #e5e5f7 20px );

  /*background-color: #e5e5f7;*/
  /*opacity: 0.8;*/
  /*background-image:  repeating-radial-gradient( circle at 0 0, transparent 0, #e5e5f7 10px ), repeating-linear-gradient( #444cf755, #444cf7 );*/

  /*background-color: #e5e5f7;*/
  /*opacity: 0.8;*/
  /*background:*/
  /*  linear-gradient(135deg, #444cf755 25%, transparent 25%) -10px 0/ 20px 20px,*/
  /*  linear-gradient(225deg, #444cf7 25%, transparent 25%) -10px 0/ 20px 20px,*/
  /*  linear-gradient(315deg, #444cf755 25%, transparent 25%) 0px 0/ 20px 20px,*/
  /*  linear-gradient(45deg, #444cf7 25%, #e5e5f7 25%) 0px 0/ 20px 20px;*/
}

.blue {
  color: #0000ff;
}

.red {
  color: #FF0000;
}

.playing-card:hover {
  transition: transform 0.5s cubic-bezier( 0.32, 1, 0.32, 1.275 );
  transform: translateY(-75px);
  box-shadow: -5px 2px 3px rgba(10, 10, 10, 0.1), 0 0 0 1px rgba(10, 10, 10, 0.1),
    -1px 1px 1px 1px red;

  z-index: 5;
}

.playing-card.backside:hover {
  transform: initial;
  /*box-shadow: -5px 2px 3px rgba(10, 10, 10, 0.1), 0 0 0 1px rgba(10, 10, 10, 0.1),*/
  /*-1px 1px 1px 1px blue;*/

  z-index: inherit;
}

.playing-card:not(.backside):hover::after {
  content: "";
  position: absolute;
  top: 100%;
  height: 75px;
  width: 100%;
}
</style>
