## TODO
- use longterm allocators for imgui
- use mem allocators for SDL

## DONE
- optimize by drawing the cell fronts and borders in less draw calls
- optimize by using texture array in shader instead of many bind texture calls
- optimize cell back drawing a bit (kinda jank)
- imgui window to display mem usage/footprint
- debug display key toggle
- custom game menu
- display bombs left
- timer counting up as you play
- display timer
- bug: after game ends, click on face and release elsewhere
- make clicking on face retain same expression until release
- start new game from Difficulty menu
- borders
- imgui menu to select difficulty
- imgui window to display fps
- select difficulty, set size based on it
- flexible layout/size
- don't leak memory - use scoped allocators
- face should look...better

