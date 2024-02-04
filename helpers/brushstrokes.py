# EIGHT BASIC COMPONENTS:
SWOOP = 1
UNDERTURN = 2
OVERTURN = 3
WAVE = 4
OVAL = 5
BROKEN_OVAL = 6
REVERSE_OVAL = 7
ASCENDING_STEM = 8
DESCENDING_STEM = 9
DOT = 10 # idk man

# focus on a-e rn
letter_dict = {
    "a": [OVAL, UNDERTURN], # etc
    "b": [ASCENDING_STEM, REVERSE_OVAL],
    "c": [BROKEN_OVAL],
    "d": [OVAL, ASCENDING_STEM],
    "e": [SWOOP, BROKEN_OVAL],
    "f": [], # dawg idk
    "g": [OVAL, DESCENDING_STEM, SWOOP],
    "h": [ASCENDING_STEM, OVERTURN],
    "i": [UNDERTURN, DOT], # add dot somehow
    "j": [DESCENDING_STEM, DOT],
    "k": [],
    "l": [ASCENDING_STEM, SWOOP],
    "m": [WAVE, OVERTURN, SWOOP],
    "n": [WAVE],
    "o": [OVAL], # offsets here
    "p": [DESCENDING_STEM, REVERSE_OVAL],
    "q": [], # uhh
    "r": [], # idk
    "s": [SWOOP, REVERSE_OVAL],
    "t": [], # tall stalk
    "u": [],
    "v": [],
    "w": [],
    "x": [],
    "y": [],
    "z": [],
    " ": [] # space lol
}

# arrays of gcode for each component
# primarily position, speed, etc.
component_dict = {
    SWOOP: [
        "S1 0 0 0 0 0"
    ],
    UNDERTURN: [
        "S1 1 0 0 0 0"
    ],
    OVERTURN: [
        "S1 2 0 0 0 0"
    ],
    WAVE: [
        "S1 3 0 0 0 0"
    ],
    OVAL: [
        "S1 4 0 0 0 0"
    ],
    BROKEN_OVAL: [
        "S1 5 0 0 0 0"
    ],
    REVERSE_OVAL: [
        "S1 6 0 0 0 0"
    ],
    ASCENDING_STEM: [
        "S1 7 0 0 0 0"
    ],
    DESCENDING_STEM: [
        "S1 8 0 0 0 0"
    ],
    DOT: [
        "S1 9 0 0 0 0"
    ]
}

def brushstroke(word, x, y):
    traj_array = []

    traj_array.append("S1 " + str(x) + " " + str(y) +" 0 0 0") # go to starting position 

    for c in word:
        # add each character
        char_components = letter_dict[c]
        for component in char_components:
            traj_array += component_dict[component]
    
    # note: need to calculate total width of char, but do this after actually hardcoding each one
    # add code to go to a newline possibly

    return traj_array


def up_stroke():
    return ["S0"]