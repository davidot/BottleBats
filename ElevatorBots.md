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


### MVP
Initial building has:
- floors, just amount necessary evenly spaced with one unit between them
- elevators just amount, all can reach all floors

Rules:
* travelling between floors takes 1, open + close etc. takes 1

Commands from runner:
Call made, with floor for every person -> sends just floor
Elevator has reached and cycled floor -> sends current floor, all selected floors within

Commands from algo:
Move elevator to floor (+ cycle doors)
Done
