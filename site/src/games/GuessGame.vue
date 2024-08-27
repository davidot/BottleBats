<template>
    <div class="base" @mousemove="movePicker" @click="clickPicker" @wheel="scrollPicker">
        <div ref="line" class="line">
            <div class="ticks">
                <div v-for="i in numTicks + 1" class="tick" :style="{left: 'calc(' + Math.round((i - 1) / numTicks * 10000) / 100 + '% - 2px)'}"><span>{{ (i - 1) * 100 }}</span></div>
            </div>
            <div v-if="pickerLocation != null" class="picker" :style="{left: locationToCSS(pickerLocation), '--picker-color': locked ? 'red' : 'blue'}">
                <span>{{ pickerLocation }}</span>
            </div>

            <div v-for="miss in misses" class="miss" :style="{left: locationToCSS(miss)}">
                <span v-show="pickerLocation === miss">
                    {{ miss }}
                </span>
            </div>

            <div class="blocker" :style="{left: '0', right: locationToCSS(rangeEnd - rightMostHigher, 0)}">
                <div style="position: relative; left: 100%; width: fit-content;">
                    <span v-show="pickerLocation != null && pickerLocation <= rightMostHigher" style="top: 15px;">
                        &gt; {{ rightMostHigher }}
                    </span>
                </div>
            </div>

            <div class="blocker" :style="{left: locationToCSS(leftMostLower), right: '0'}">
                <div style="position: relative; right: 0; width: fit-content;">
                    <span v-show="pickerLocation != null && pickerLocation >= leftMostLower" style="top: 15px;">
                        &lt; {{ leftMostLower }}
                    </span>
                </div>

            </div>
        </div>

        <div v-if="locked" style="position: absolute; top: 0; left: 50%; color: gray;"><span style="position: relative; left: -50%;;">Click to unlock!</span></div>
    </div>
    <button style="width: 100%; margin-top: 5px;" @click="sendMessage" :disabled="pickerLocation == null">Guess!</button>
    LML: {{ leftMostLower }}
    RMH: {{rightMostHigher}}

    {{ gameMessages.at(-1) }}

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
            rangeStart: 0,
            rangeEnd: 1000,
            // Those should be part of the game settings

            numTicks: 10,
            pickerLocation: null,
            locked: true,
        };
    },
    computed: {
        gameMessages() {
            return this.messages
                .filter(m => m.from === "game" && m.content.startsWith("result "))
                .map(m => {
                    const parts = m.content.replace("result ", "").split(" ");
                    return [parseInt(parts[0]), parts[1]];
                });
        },
        misses() {
            return this.gameMessages.map(m => m[0]);
        },
        rightMostHigher() {
            let highest = null;
            for (const mess of this.gameMessages) {
                if (mess[1] !== "higher")
                    continue;
                if (highest == null || mess[0] > highest)
                    highest = mess[0];
            }
            return highest || this.rangeStart;
        },
        leftMostLower() {
            let lowest = null;
            for (const mess of this.gameMessages) {
                if (mess[1] !== "lower")
                    continue;
                if (lowest == null || mess[0] < lowest)
                    lowest = mess[0];
            }
            return lowest || this.rangeEnd;
        }
    },
    methods: {
        locationToCSS(number, offset=2) {
            return 'calc(' + Math.round((number / this.rangeEnd) * 10000) / 100 + '% - ' + offset + 'px)'
        },
        movePicker(ev) {
            if (this.locked)
                return;

            const lineElement = this.$refs.line;
            const boundingBox = lineElement.getBoundingClientRect();

            const offset = ev.clientX - boundingBox.x - 1;
            if (offset < 0 || offset > boundingBox.width) {
                this.pickerLocation = null;
                return;
            }

            this.pickerLocation = Math.round((offset / boundingBox.width) * this.rangeEnd);
        },
        clickPicker() {
            this.locked = !this.locked;
        },
        scrollPicker(ev) {
            this.deltaPicker(ev.wheelDeltaY);
            ev.preventDefault();
        },
        deltaPicker(change) {
            this.pickerLocation += Math.sign(change);
            this.pickerLocation = Math.min(this.rangeEnd, Math.max(0, this.pickerLocation));
        },
        sendMessage() {
            if (this.pickerLocation == null)
                return;
            const value = this.pickerLocation + '';
            this.pickerLocation = null;
            this.$emit('sendMessage', value);
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
}

.line {
    width: 90%;
    position: relative;
    left: 5%;
    bottom: -50%;
    height: 1px;
}

.line::after {
    content: '';
    position: relative;
    display: block;
    border: 1px solid black;
    /* border-left: 0; */
    /* border-right: 0; */
    /* left: -2px; */
    /* left: 25px; */
    /* border-right: 2px solid black; */
}

.ticks > .tick {
    border-left: 2px solid black;
    height: 8px;
    position: absolute;
}

.ticks > .tick:first-of-type, .ticks > .tick:last-of-type {
    height: 16px;
}

.tick > span {
    top: 8px;
    color: gray;
}

.tick:first-of-type > span, .tick:last-of-type > span {
    top: 16px;
    color: gray;
    font-weight: bold;
}

.picker {
    position: absolute;
    float: left;
    top: -10px;
    height: 20px;
    border-left: 2px solid var(--picker-color);
}

.picker > span {
    top: -100%;
    color: var(--picker-color);
}

.miss {
    border-left: 2px solid orange;
    position: absolute;
    height: 12px;
}

.miss > span {
    bottom: -100%;
    color: orange;
    font-weight: bold;
}

.base div span {
    position: relative;
    left: -50%;
}

.blocker {
    position: absolute;
    color: pink;
    height: 10px;
    border-left: 2px solid pink;
    background: repeating-linear-gradient(-45deg, pink, pink 5px, white 5px, white 10px);
    transition: all 2s ease-out;
    transition-property: left, right;
}

</style>