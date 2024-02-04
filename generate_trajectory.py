import json 
from helpers.helpers import process_stroke

config_file = open("config.json")
config = json.load(config_file)

target_file = open("trajectory.txt", "w")

# homing?
# add gcode for start coords
# also speed stuff


strokes = config["strokes"]
for s in strokes:
    traj = process_stroke(s) # array of locations
    for line in traj:
        target_file.write(line + "\n")

target_file.close()