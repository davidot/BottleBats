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

            <component :is="'div'" v-for="(nm, index) in things" :key="nm" class="anim-item" :data-change="index <= (oldIndices[index] || -1) ? 'pos' : 'neg'">
              <span class="name">
                {{ nm.nm }}
              </span>
              {{ index }} vs {{ oldIndices[index] || -1 }}
            </component>
          </TransitionGroup>
        </div>
      </div>
    </div>
  </div>
</template>

<script>

export default {
  name: "LeaderBoard",
  data() {
    return {
      count: 1,
      things: [{ nm: "hi" }, {nm: "hello"}, {nm: "bye"}],
      oldIndices: [],
    };
  },
  methods: {
    fillOldIndices() {
      this.oldIndices = [...this.things.keys()];
    },
    addMe() {
      this.things.push({nm: this.things[this.things.length - 1].nm + 'b'});

      this.fillOldIndices();
    },
    removeMe() {
      this.fillOldIndices();

      const index = Math.random() * this.things.length;
      this.things.splice(index, 1);
    },
    shuffle() {
      this.fillOldIndices();

      for (let i = 0; i < this.things.length; i++) {
        let number = Math.floor(i + (this.things.length - i) * Math.random());
        const temp = this.things[i];
        this.things[i] = this.things[number];
        this.things[number] = temp;

        const tempIdx = this.oldIndices[i];
        this.oldIndices[i] = this.oldIndices[number];
        this.oldIndices[number] = tempIdx;

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

.moving-item[data-change="neg"] {
  color: red;
}

/*.anim-item:hover .name {*/
/*  transform: scale(120%);*/
/*}*/

.name:hover {
  /*display: inline-block;*/
  transform: scale(150%);
}

.anim-item:first-child {
  transform: scale(125%) translateY(25%);
  transform-origin: 0 0;
  margin-bottom: 10px;
}

</style>
