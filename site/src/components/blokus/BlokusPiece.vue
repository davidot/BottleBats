<template>
    <table
        :style="{'top': `calc(${piece.top} * 32px)`, 'left': `calc(${piece.left} * 32px)`}">
        <tr v-for="i in piece.height">
            <td v-for="j in piece.width" :class="[activeSet.has(`${i-1},${j-1}`) || 'empty']" 
                :data-row="piece.top + i - 1" :data-col="piece.left + j - 1">
            </td>
        </tr>
    </table>
    
</template>

<script>
export default {
    props: {
        piece: Object,
    },
    computed: {
        activeSet() {
            if (this.piece.active != null)
                return this.piece.active;
            const active = new Set();

            for (let y = 0; y < this.piece.height; ++y) {
                for (let x = 0; x < this.piece.width; ++x){
                    if (this.piece.onOff[y * this.piece.width + x] === 1)
                        active.add(`${y},${x}`);
                }
            }

            return active;
        },
    },
};
</script>