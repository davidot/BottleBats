<template>
  <div>
    <p>
      Welkom bij BattleBots.
      Vandaag spelen we <b>Vijf</b>.
    </p>


    <h4>
      Regels
    </h4>
    <p>
    </p>

    <h4>
      Bot regels
    </h4>
    <div>
      Als bot krijg je berichten via stdin (System.in, etc.). <br />
      En moet je antwoorden via stdout (System.out, print, etc.) <br />

      Alle berichten zijn een of meerdere lijnen text met '\n' als eind. (Dit gaat waarschijnlijk vanzelf goed)

      Alle berichten van de runner beginnen met een lijn van het formaat:
      <pre>[type] [lines] [extra data]</pre>
      Hier is type het type bericht, lines de hoeveelheid lines die hierna nog deel zijn van dit bericht
      en extra data is alvast wat informatie.

      In het begin krijg je een bericht dat controleerd of je klaar bent.
      <pre>game 0 vijf</pre>
      Hierop moet je <pre>ready</pre> reageren.

      Daarna krijg je alleen nog maar berichten van de vorm:
      <ul>
        <li> <pre>died</pre> het spel is afgelopen of je hebt iets fout gedaan</li>
        <li> <pre style="white-space: pre-line">turn 7 [positie] [spelers levend] [ronde]
              [#kaarten in hand van speler 0] [kaarten]
              [#kaarten in hand van speler 1] [kaarten]
              [#kaarten in hand van speler 2] [kaarten]
              [#kaarten in hand van speler 3] [kaarten]
              [#kaarten in hand van speler 4] [kaarten]
              [#afgeledge kaarten] [kaarten]
              [#kaarten nog in de pot] [kaarten]
            </pre>
          Als je een zet moet maken, met de volgende betekenis
          <ul>
            <li><code>[positie]</code> Waar je aan tafel zit, van 0 tot en met 4 (0 begint)</li>
            <li><code>[spelers levend]</code> Hoeveel spelers er nog levend zijn (die hebben dus 0 kaarten)</li>
            <li><code>[ronde]</code> De hoeveelste ronde het is (niet echt nuttig maar ja)</li>
            <li><code>[#iets kaarten]</code> Een getal wat aangeeft hoeveel kaarten er volgen</li>
            <li><code>[kaarten]</code> De kaarten gecodeerd als volgt:
              <table style="border-collapse: collapse; margin-top: 4px; margin-bottom: 4px;">
                <tr class="side-border">
                  <td>Kaart </td><td v-for="a in cardMap" :key="'kaart-' + a[0]">{{ a[0] }}</td>
                </tr>
                <tr class="side-border" style="border-top: 1px solid black">
                  <td>Teken </td><td v-for="a in cardMap" :key="'char-' + a[0]">{{ a[1] }}</td>
                </tr>
              </table>
            </li>
          </ul>
        </li>
      </ul>
    </div>


    <h4>
      Input en output voorbeelden
    </h4>
    <div>
      Hier betekent <code> &gt; </code> van de runner naar de bot en <code> &lt; </code> een antwoord vanuit de bot
      <br>
      <pre style="white-space: pre-line">
        &gt; game 0 vijf
        &lt; ready

        &gt; turn 7 0 5 0
        &gt; 3 679
        &gt; 3 23Q
        &gt; 3 4QK
        &gt; 3 3QQ
        &gt; 3 78J
        &gt; 0
        &gt; 40 +**AAAA22233444555566677888999TTTTJJJKKK
        &lt; play 7

        &gt; turn 7 0 2 1
        &gt; 9 *A234699J
        &gt; 4 36JQ
        &gt; 0
        &gt; 0
        &gt; 0
        &gt; 16 22345556778JQQQK
        &gt; 26 +*AAA2344567788899TTTTJKKK
        &lt; play A

        &gt; turn 7 0 2 2
        &gt; 9 *2346699J
        &gt; 14 *AA33678TJQKKK
        &gt; 0
        &gt; 0
        &gt; 0
        &gt; 18 A22345556778JJQQQK
        &gt; 14 +A244578899TTT
        &lt; play 4

        &gt; died
      </pre>
    </div>


    <h4>
      Code voorbeelden
    </h4>
    <ul>
      <li><a href="examples/example.java" download>Java</a></li>
      <li><a href="examples/example.py" download>Python</a></li>
      <li><a href="examples/example.cpp" download>C++</a></li>
      <li><a href="examples/test-vijf.exe" download>Test programm</a></li>
    </ul>


  </div>
</template>

<script>
export default {
  name: "ExplainView",
  data() {
    return {
      cardMap: [
        ['Aas', 'A'],
        ['2', '2'],
        ['3', '3'],
        ['4', '4'],
        ['5', '5'],
        ['6', '6'],
        ['7', '7'],
        ['8', '8'],
        ['9', '9'],
        ['10', 'T'],
        ['Boer', 'J'],
        ['Vrouw', 'Q'],
        ['Heer', 'K'],
        ['Joker', '*'],
        ['Regelkaart', '+'],
      ]
    }
  }
}
</script>

<style scoped>

h4 {
  margin-bottom: .5em;
}

p {
  margin-top: 0;
}

.side-border {
  border: 1px solid black;
}

.side-border td {
  border-right: 1px solid black;
  text-align: center;
  min-width: 1em;
  padding-left: 0.2em;
  padding-right: 0.2em;
}

</style>
