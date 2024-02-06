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
        "000000010000000100000001000000010000000"
    ],
    UNDERTURN: [
        "000000010000000100000001000000010000000"
    ],
    OVERTURN: [
        "000000010000000100000001000000010000000"
    ],
    WAVE: [
        "000000010000000100000001000000010000000"
    ],
    OVAL: [
        "000000010000000100000001000000010000000"
    ],
    BROKEN_OVAL: [
        "000000010000000100000001000000010000000"
    ],
    REVERSE_OVAL: [
        "000000010000000100000001000000010000000"
    ],
    ASCENDING_STEM: [
        "000000010000000100000001000000010000000"
    ],
    DESCENDING_STEM: [
        "000000010000000100000001000000010000000"
    ],
    DOT: [
        "000000010000000100000001000000010000000"
    ]
}

def brushstroke(word, x, y):
    traj_array = []

    # traj_array.append("S1 " + str(x) + " " + str(y) +" 0 0 0") # go to starting position 

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