import numpy as np
import matplotlib.pyplot as plt



Ax, Ay = 2,         0
Bx, By = 1, np.sqrt(3)

D = 216
R = D / 2
R_squared = R * R

t = np.linspace(0, np.pi*2)
xs = R * np.cos(t)
ys = R * np.sin(t)

margin = .1

def circle(centre, radius):
    c = plt.Circle(centre, radius)
    ax.add_artist(c)


def rod_sector(r, gap, rotation_in_turns):
    d = 2 * r
    ox = gap * np.cos(np.pi/6) + r * np.sqrt(3)
    oy = gap * np.sin(np.pi/6) + r
    a = 0
    while True:
        b = 0
        didB = False
        while True:
            x = (a*Ax + b*Bx) * d + ox
            y = (a*Ay + b*By) * d + oy
            if np.sqrt(x*x + y*y) + r + margin >= R:
                break
            x,y = rotate((x,y), rotation_in_turns)
            circle((x,y), d/2)
            b += 1
            didB = True
        a += 1
        if not didB:
            break

def rotate(p, turns):
    radians = np.pi * 2 * turns
    X,Y = p
    cos = np.cos(radians)
    sin = np.sin(radians)
    x = cos*X - sin*Y
    y = sin*X + cos*Y
    return x,y

fig, ax = plt.subplots()
ax.set_aspect(1)

deluxe       = (4.8, 6.4, 7.9, 9.5, 11.1, 12.7)
ultra_deluxe = (3.2, 4.8, 6.4, 7.9,  9.5, 11.1)

for t, d in zip(range(6), reversed(ultra_deluxe)):
    rod_sector(d/2, 14.4, t/6)


ax.set_xlim(-R, R)
ax.set_ylim(-R, R)
plt.plot(xs, ys)
plt.show()
