<template>
  <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 300 300" :class="['logo-svg-main', animating && 'animating', (popping) && 'popping']">
    <g>
      <g id="bb-logo">
        <!--            <rect class="cls-1" width="299.12" height="299.12"/>-->
        <path
            class="wing-top"
            d="M191.51,142.41A248.66,248.66,0,0,1,261,15c-6.17,22.92-12.93,46.05,10.5,62.2-28.27,11.31-37,26-27.46,46.05C219.15,118.57,201.61,130.91,191.51,142.41Z"
        />
        <path
          class="wing-bottom"
          d="M142.41,191.51A248.66,248.66,0,0,0,15,261c22.92-6.17,46.05-12.93,62.2,10.5,11.31-28.27,26-37,46.05-27.46C118.57,219.15,130.91,201.61,142.41,191.51Z"
        />
        <path
          class="bottle-lighter"
          d="M214.17,214.17,195.36,233a8.35,8.35,0,0,1-11.16.59l-69.83-56.4,31.4-31.4Z"
        />
        <path
          class="bottle-lighter"
          d="M115.3,162.38c-3.89,9.36-3,10.25-.93,14.79L147.1,147.1l-9.26-9.26"
        />
        <path
          class="bottle-lightest"
          d="M214.17,214.17l18.58-19.11a8.48,8.48,0,0,0,.52-11.23l-56.1-69.46-31.4,31.4Z"
        />
        <path
          class="bottle-lightest"
          d="M99.31,113A84.81,84.81,0,0,1,113,128.29c3.5,5,5.67,8.24,6.69,12.82,2,9.09-1.94,17.24-4.35,21.27a32.54,32.54,0,0,0,9.09-.37c12.62-2.32,20.2-11.54,22.71-14.91l-37.62-37.62Z"
        />
        <path
          class="bottle-dark"
          d="M211.17,197.38l-51.25-68.85-34.25,33.26L159.6,188.4C182.72,188.05,202.68,193,211.17,197.38Z"
        />
        <path
          class="bottle-dark"
          d="M231.27,181.36l-54.1-67-31.4,31.4,29.5,29.5C198.39,174.92,222.78,177,231.27,181.36Z"
        />
        <path
          class="bottle-dark"
          d="M162.38,115.3c9.36-3.89,10.25-3,14.79-.93L147.1,147.1l-9.26-9.26"
        />
        <path
          class="bottle-lighter"
          d="M103.28,112.49a103.84,103.84,0,0,1,14.46,14.74,48.16,48.16,0,0,1,3,4.27,43.42,43.42,0,0,1,5.68,14.35,37.33,37.33,0,0,1-.73,15.94c9.78-4,19-11.43,21.43-14.69l-37-37Z"
        />
        <path
          class="bottle-lighter"
          d="M113,99.31A84.81,84.81,0,0,0,128.29,113c5,3.5,8.24,5.67,12.82,6.69,9.09,2,17.24-1.94,21.27-4.35-.13,7.38-.19,7.45-2.46,13.23-4.13,10.55-7.32,13.25-12.82,18.57l-37.62-37.62Z"
        />
        <path
          class="liquid-dark"
          d="M126.91,187.12l107.27-1.64a9.22,9.22,0,0,1,.19,7.21,8,8,0,0,1-1.62,2.37L195.36,233a8.28,8.28,0,0,1-2.84,1.85,8.57,8.57,0,0,1-5.93.08,8.2,8.2,0,0,1-2.41-1.37Z"
        />
        <path
          class="liquid-light"
          d="M234.18,185.48c0,5.77-24.16,10.45-54,10.45s-54-4.68-54-10.45,24.17-10.46,54-10.46S234.18,179.7,234.18,185.48Z"
        />
        <rect
          class="reflection reflection-fade"
          x="145.75"
          y="157.97"
          width="7.61"
          height="69.93"
          rx="3.81"
          transform="translate(-94.55 192.43) rotate(-52.1)"
        />
        <rect
          class="reflection"
          x="183.01"
          y="214.28"
          width="7.61"
          height="14.43"
          rx="3.81"
          transform="translate(-102.72 232.84) rotate(-52.1)"
        />
        <ellipse
            class="bottle-dark"
            cx="0.1936598"
            cy="150.17273"
            rx="9.8900003"
            ry="5.6399999"
            transform="rotate(-45)"
            id="ellipse42-3" />
        <g class="bottle-top">
          <ellipse
              class="liquid-dark"
              cx="110.12"
              cy="110.12"
              rx="9.89"
              ry="5.64"
              transform="translate(-45.62 110.12) rotate(-45)"
          />
          <rect
              class="liquid-dark"
              x="94.25"
              y="95.68"
              width="19.78"
              height="16.93"
              transform="translate(-43.14 104.14) rotate(-45)"
          />
          <ellipse
              class="liquid-light"
              cx="98.15"
              cy="98.15"
              rx="9.89"
              ry="5.64"
              transform="translate(-40.66 98.15) rotate(-45)"
          />
        </g>

      </g>
    </g>
  </svg>
</template>

<script>
export default {
  name: "Logo-SVG",
  props: {
    animating: {
      default: false,
      type: Boolean
    },
    popping: {
      default: false,
      type: Boolean
    }
  }
};
</script>

<style scoped>

.wing-top {
  fill: #57965d;
}

.logo-svg-main.animating .wing-top {
  transform-box: fill-box;
  transform-origin: left bottom;
  animation: flap linear 1s infinite;
  /*transform: rotate(-10deg);*/
}

@keyframes flap {
  from {
    transform: rotate(0deg);
  }

  50% {
    transform: rotate(-15deg);
  }

  to {
    transform: rotate(0deg);
  }
}

.wing-bottom {
  fill: #3e6c40;
}

.logo-svg-main.animating .wing-bottom {
  transform-box: fill-box;
  transform-origin: right top;
  animation: aflap linear 1s infinite;
  /*animation-fill-mode: both;*/
}

@keyframes aflap {
  from {
    transform: rotate(-15deg);
  }

  50% {
    transform: rotate(0deg);
  }

  to {
    transform: rotate(-15deg);
  }
}

.bottle-lighter {
  fill: #f5b47d;
}

.bottle-lightest {
  fill: #f9d0ad;
}

.bottle-dark {
  fill: #f59e56;
}

.liquid-dark {
  fill: #afc160;
}

.liquid-light {
  fill: #d7e18c;
}

.reflection {
  fill: #fff;
}

.reflection-fade {
  opacity: 0.48;
}

.logo-svg-main.popping .bottle-top {
  animation: pop 1s linear infinite;
  transform-box: content-box;
}

@keyframes pop {
  from {
    transform: translate(0, 0);
  }

  50% {
    transform: translate(-10%, -10%);
  }

  to {
    transform: translate(0, 0);
  }
}
</style>
