## Bottlebats II

### Limitations (at least for initial prototype)

- No size/weight for elevator and passengers
- No time to get in/out of elevator, if stopped take x time to open + unload + load + close in fixed time (per elevator?? probably not even)
- No groups, all elevators from one floor can go to the same floors
- No acceleration
- Elevators can never be stationary between floors (probably always true)
- No height all floors are evenly spaced by id
- No timer command
- Must open doors on every send command
- All elevators can reach all floors
- Just info that a button is pressed on floor
- No reactive generation of request!!! (complicates lots of things **:(** )


### MVP
_Italics means nice but not actually needed for mvp_

#### building
- floors, just amount necessary evenly spaced with one unit between them
- elevators just amount, all can reach all floors

####Rules:
* travelling between floors takes 1, open + close etc. takes 1

####Commands from runner:
- Call made, with floor for every person -> sends just floor
- Elevator has reached and cycled floor -> sends current floor, all selected floors within

#### Commands from algo:
- Move elevator to floor (+ cycle doors), if elevator already moving overwrite
- Done


#### Tracking:
- Wait times until on elevator
- Total wait + travel time
- Extra travel time
- Total elevator travel distance
- Total elevator door open + close times
- _Rollercoaster event (reversing direction of elevator)_
- _Elevator moving with no one inside_

Compute: 
- max waiting time
- avg waiting time
- avg extra travel
- max extra travel
- _Power usage: total elevator travel distance + doors opening + (moving without anyone extra here??)_


### Bot structure
- Run in container
- No internet
- Can theoretically store files? no memory for now
- No root (different user even?)
- Must be linux based
- Must have entrypoint set

Protocol:
```
> elevated
> setting capacity on|off
> setting info low|high|min|max
> setting commands basic|routing|...
> building [#groups] [#elevators]
 > group [group_id] [#floors] [...floors...] * #groups
 > elevator [elevator_id] [group_id] [speed] [capacity] [door_open_time] [door_close_time] * #elevators
> done
< ready | reject
> events [time] [#events]
 > timer
 > closed [elevator_id] [group_id] [current_height] [#target floors] [...targets...] {extra_info1}
 > request [height] [group_id] {extra_info2}
 
info low -> 
        {extra_info2} = [direction = up|down]

< Second
``` 
