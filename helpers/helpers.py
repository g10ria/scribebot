from helpers.calibration import *
from helpers.brushstrokes import brushstroke
from helpers.random_walk import random_walk

def process_stroke(stroke):
    type = stroke["type"]
    if (type == 0): return brushstroke(stroke["value"], stroke["x"], stroke["y"])
    if (type == 1): return random_walk() # inputs