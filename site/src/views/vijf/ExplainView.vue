<template>
  <div>
    <p>
      Welkom bij BattleBots.
      Vandaag spelen we <b>Vijf</b>.
    </p>

    <h3>
      Snelle linkies
      <ul>
        <li><a href="#explain-regels">Regels</a></li>
        <li><a href="#explain-bot">Bot regels</a></li>
        <li><a href="#explain-run">Input en output voorbeelden</a></li>
        <li><a href="#explain-example">Code voorbeelden en tools</a></li>
      </ul>

    </h3>


    <h3 id="explain-regels">
      Regels
    </h3>
    <div>
      <h5>Je bent af als je een vijf krijgt.</h5>


      <h4>
        Kaarten uitdelen
      </h4>

      Omstebeurt wordt aan iedereen drie kaarten uitgedeeld.
      Zit er een vijf tussen de kaarten dan gaan de vijf(en) terug en krijg je nog zoveel kaarten.
      Zit daar een vijf tussen? Helaas <b style="color: red">dood</b>.


      <h4>
        Kaarten spelen
      </h4>
      <p>Je bent in een rondje aan de beurt (als je nog niet af bent).
      Je moet een kaart spelen, speel je alles behalve een regel kaart dan trek je zoveel kaarten als het getal op de kaart.
        Met de speciale kaarten: Joker = 0, Aas = 1, Boer = 11, Vrouw = 12, Heer = 13.</p>

      <p>Kom je een vijf tegen oepsie doepsie <b style="color: red">dood</b> en trek niet meer kaarten, volgende aan de beurt.
      Kom je een regel kaart tegen, joepie stop met kaarten trekken.</p>

      <p>Heb je net je laatste kaart gespeeld en nu een lege hand aan het einde van je beurt?
      Hopelijk ben je als enige over want anders <b style="color: red">dood</b>.</p>

      <h4>
        Regelkaart
      </h4>

      <p>De regelkaart is speciaal bij het trekken van de kaart en het spelen.
        Bij het trekken is het net zoals een 5 dat je moet stoppen. </p>

      <p>Bij het spelen ga je in de spel richting alle nog spelers die nog niet af zijn langs en moeten ze twee kaarten trekken.
        Zit er een vijf tussen helaas <b style="color: red">dood</b>. </p>
    </div>

    <h3 id="explain-bot">
      Bot regels
    </h3>
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


    <h3 id="explain-run">
      Input en output voorbeelden
    </h3>
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


    <h3 id="explain-example">
      Code voorbeelden en tools
    </h3>
    <ul>
      <li><a href="vijf-examples/example.java" download>Java</a></li>
      <li><a href="vijf-examples/example.py" download>Python</a></li>
      <li><a href="vijf-examples/example.cpp" download>C++</a></li>
      <li><a href="vijf-examples/test-vijf.exe" download>Test programma</a></li>
    </ul>

    <h3>
      Test programma
    </h3>
    <div>
      Run als <code>test-vijf.exe COMMANDO OM TE RUNNEN HIER</code>
      Bijvoorbeeld
      <code>test-vijf.exe python.exe run.py</code>
    </div>
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
