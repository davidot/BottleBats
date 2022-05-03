<template>
  Elevated uitleg hier!
  <pre>
    &gt; elevated
    &gt; setting capacity on|off
    &gt; setting info low|high|min|max
    &gt; setting commands basic|routing|...
    &gt; building [#groups] [#elevators]
    &gt; group [group_id] [#floors] [...floors...] * #groups
    &gt; elevator [elevator_id] [group_id] [speed] [capacity] [door_open_time] [door_close_time] * #elevators
    &gt; done
    &lt; ready | reject &lt;reason&gt;
    &gt; events [time] [#events]
    &gt; timer
    &gt; closed [elevator_id] [group_id] [current_height] [#target floors] [...targets...] {extra_info1}
    &gt; request [height] [group_id] {extra_info2}
    [
      info low ->
      {extra_info1} = still-waiting [-|up|down|up,down]
      {extra_info2} = [direction = up|down]
    ]
    &gt; done
    &lt; set-timer [time]
    &lt; move [elevator_id] [target]
    &lt; done
  </pre>

  <a href="elevated-examples/random_travel.py" download>Python</a>
  <a href="elevated-examples/elevated-tester.exe" download>Local tester</a>
</template>

<script>
export default {
  name: "ExplainView"
}
</script>

<style scoped>

</style>
