import random
counts = {"bad": 0, "good": 0, "neutral": 0}

x = random.randint(0, 50)
if x == 0:
    print("Hello Beth")
    counts["neutral"] += 1
elif x == 1:
    print("Beth is mean and rude")
    counts["bad"] += 1
else:
    print("Beth is the best girlfriend in the whole world")
    counts["good"] += 1

