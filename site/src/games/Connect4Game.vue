<template>
    <div class="base" style="--animation-time: 1s; --hover-col: #aaaaaa; --filled-col: #222222; --client-side-col: #888888;">
        <table :class="['game', turnFor + '-turn']" v-if="values != null" @mouseleave="hoverOn(null)">
            <tr v-for="i in 6" :class="[lowestHover == i ? 'hover-row' : '']">
                <td v-for="j in 7" :class="['cell', values[coordToIndex(i, j)] + '-cell', hoveringOn == (j - 1) ? 'hover-col' : '']" @mouseover="hoverOn(j - 1)" @click="send(j - 1)">
                </td>
            </tr>
        </table>
        <div v-else>
            Waiting for game state...
        </div>
    </div>
</template>

<script>

export default {
    props: {
        messages: {
            type: Array,
            required: true,
        },
        // FIXME: Get "game" settings from the config
    },
    emits: ['suggestion', 'sendMessage'],
    data() {
        return {
            hoveringOn: null,
        };
    },
    computed: {
        lastGameStateIndex() {
            const index = this.messages.findLastIndex(m => m.from === "game" && m.content.startsWith("turn "));            
            return index == -1 ? null : index;
        },
        lastGameState() {
            if (this.lastGameStateIndex == null)
                return null;
            const lastMessage = this.messages[this.lastGameStateIndex];
            return lastMessage.content.toLowerCase().split(" ");
        },
        extraMove() {
            if (this.lastGameStateIndex == null)
                return null;
            const maybeLastMove = this.messages.findLast((m, idx) => idx > this.lastGameStateIndex && m.from.endsWith("user") && !Number.isNaN(Number(m.content)));
            if (maybeLastMove == null)
                return null;
            const probablyTurn = this.lastGameState[1];

            const col = Number(maybeLastMove.content);
            const row = this.lowestEmptyInCol(col);

            return {
                "at": row * 7 + col,
                "symbol": probablyTurn,
            };
        },
        turnFor() {
            if (this.lastGameState == null)
                return null;

            return this.lastGameState[1];
        },
        values() {
            if (this.lastGameState == null)
                return null;

            const str = this.lastGameState[2];
            const values = [];
            for (let line of str.split('|')) {
                for (let c of line) {
                    if (c == '-') {
                        values.push('empty');
                    } else {
                        values.push(c);
                    }
                }
            }

            if (this.extraMove != null) {
                if (values[this.extraMove.at] == 'empty') {
                    values[this.extraMove.at] = 'clientside ' + this.extraMove.symbol;
                }
            }
            return values;
        },
        lowestHover() {
            if (this.hoveringOn == null || this.values == null || this.values[this.hoveringOn] != 'empty')
                return null;

            let lowest = this.lowestEmptyInCol(this.hoveringOn);
            if (lowest == null)
                return null;
            return lowest + 1;
        }
    },
    methods: {
        lowestEmptyInCol(col) {
            for (let i = 5; i >= 0; --i) {
                if (this.values[i * 7 + col] == 'empty')
                    return i;
            }
            return null;
        },
        coordToIndex(i, j) {
            return (i - 1) * 7 + (j - 1);
        },
        hoverOn(index) {
            if (index == null || this.values == null || this.values[index] != 'empty') {
                this.$emit('suggestion', '');
                this.hoveringOn = null;
                return;
            }

            this.hoveringOn = index;
            this.$emit('suggestion', index + '');
        },
        send(index) {
            if (index == null || this.values == null || this.values[index] != 'empty') {
                return;
            }
            this.$emit('sendMessage', index + '');
        }
    },
};

</script>

<style scoped>

.base {
    width: 95%;
    min-height: 100px;
    margin: auto;
    background-color: white;
    display: flex;
    flex-direction: row;
    align-items: center;
    position: relative;
    justify-content: center;
    padding: 10px 0;
}

.game {
    border-collapse: collapse;
}

.cell {
    width: 32px;
    height: 32px;
    border: 1px solid black;
    box-sizing: content-box;
    position: relative;
}

/* .x-turn .empty-cell:hover::before,
.x-turn .empty-cell:hover::after, */
.x-cell::before,
.x-cell::after {
    content: '';

    position: absolute;
    top: 0;
    left: 0;

    width: 4px;
    height: 120%;

    transform-origin: 0% 0%;
}

.x-turn .empty-cell:hover::before,
.x-turn .empty-cell:hover::after {
    background-color: var(--hover-col);
}

.clientside.x-cell::before,
.clientside.x-cell::after {
    background-color: var(--client-side-col);
}

.clientside.o-cell::before {
    border-color: var(--client-side-col);
}


.x-cell::before, .x-cell::after {
    background-color: var(--filled-col);
    animation: xClip calc(var(--animation-time) / 2) linear forwards;
    visibility: hidden;
}

@keyframes xClip {
    0% {
        height: 0%;
        visibility: visible;
    }
    100% {
        height: 120%;
        visibility: visible;
    }
}

.x-turn .empty-cell:hover::before,
.x-cell::before {
    transform: translate(1px, 4px) rotate(-45deg);
    animation-delay: calc(var(--animation-time) / 2);
}

.x-turn .empty-cell:hover::after,
.x-cell::after {
    transform: translate(27px, 4px) rotate(45deg) translate(0, -4px);
}


/* .o-turn .empty-cell:hover::before, */
.o-cell::before {
    content: '';
    box-sizing: border-box;
    position: absolute;
    border-radius: 50%;
    border: 4px solid;
    width: 90%;
    height: 90%;
    top: 5%;
    left: 5%;
    display: inline-block;
}

.o-turn .empty-cell:hover::before {
    border-color: var(--hover-col);
}

.o-cell::before {
    animation: circleClip var(--animation-time) linear;
    border-color: var(--filled-col);
}

@keyframes circleClip {
    0%    {clip-path:polygon(50% 50%, 50% 0, 50% 0, 50% 0, 50% 0, 50% 0, 50% 0)}
    12.5% {clip-path:polygon(50% 50%, 50% 0, 100% 0, 100% 0, 100% 0, 100% 0, 100% 0)}
    37.5% {clip-path:polygon(50% 50%, 50% 0, 100% 0, 100% 100%, 100% 100%, 100% 100%, 100% 100%)}
    62.5% {clip-path:polygon(50% 50%, 50% 0, 100% 0, 100% 100%, 0 100%, 0 100%, 0 100%)}
    87.5% {clip-path:polygon(50% 50%, 50% 0, 100% 0, 100% 100%, 0 100%, 0 0, 0 0)}
    100%  {clip-path:polygon(50% 50%, 50% 0, 100% 0, 100% 100%, 0 100%, 0 0, 50% 0)}
}


.cell.hover-col {
    background-color: #eeeeee;
}

.hover-row .cell.hover-col {
    background-color: red;
}

.cell:last-child {
    border-right: 0;
}

.cell:first-child {
    border-left: 0;
}

.game > tr:first-child > td {
    border-top: 0;
}

.game > tr:last-child > td {
    border-bottom: 0;
}

.game > tr:first-child > td:first-child {
    border-top-left-radius: 25%;
}

.game > tr:first-child > td:last-child {
    border-top-right-radius: 25%;
}

.game > tr:last-child > td:first-child {
    border-bottom-left-radius: 25%;
}

.game > tr:last-child > td:last-child {
    border-bottom-right-radius: 25%;
}


</style>