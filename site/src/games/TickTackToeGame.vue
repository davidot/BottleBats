<template>
    <div class="base">
        <table :class="['game', turnFor + '-turn']" v-if="values != null" @mouseleave="hoverOn(null)">
            <tr v-for="i in 3">
                <td v-for="j in 3" :class="['cell', values[coordToIndex(i, j)] + '-cell']" @mouseover="hoverOn(coordToIndex(i, j))" @click="send(coordToIndex(i, j))">
                    &nbsp;
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
        lastGameState() {
            const maybeLastMesasge = this.messages
                .filter(m => m.from === "game" && m.content.startsWith("turn "))
                .at(-1);
            if (maybeLastMesasge == null)
                return null;
            return maybeLastMesasge.content.toLowerCase().split(" ");
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
            return values;
        }
    },
    methods: {
        coordToIndex(i, j) {
            return (i - 1) * 3 + (j - 1);
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
    watch: {
        pickerLocation(newValue) {
            if (newValue == null) {
                this.$emit('suggestion', '');
            } else {
                this.$emit('suggestion', newValue + '');
            }
        }
    }
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
    width: 2em;
    height: 2em;
    border: 1px solid black;
}

.x-turn .empty-cell:hover::after {
    content: 'X';
    font-size: larger;
    font-weight: bold;
}

.o-turn .empty-cell:hover {
    content: 'O';
    font-size: larger;
    font-weight: bold;
}

.x-cell::after {
    content: 'X';
}

.o-cell::after {
    content: 'O';
}

.cell:hover {
    background-color: #eeeeee;
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