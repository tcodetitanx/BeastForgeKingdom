"""Drive the running game window: screenshot / click / key, via pyautogui.
Usage: python drive.py shot NAME | click X Y | rclick X Y | key KEY | type TEXT | wait SECS
Coordinates are in GAME-window space (window at 100,80 borderless-ish; we add offsets).
"""
import sys, time
import pyautogui

pyautogui.FAILSAFE = False
SHOT_DIR = r"D:/UEprojects/BeastForgeKingdom/SourceArt/shots"
WIN_X, WIN_Y = 100, 80          # window client offset guess (title bar ~30px)
TITLE_H = 30

def game_to_screen(x, y):
    return WIN_X + int(x), WIN_Y + TITLE_H + int(y)

cmds = sys.argv[1:]
i = 0
while i < len(cmds):
    c = cmds[i]
    if c == "shot":
        import os
        os.makedirs(SHOT_DIR, exist_ok=True)
        path = f"{SHOT_DIR}/{cmds[i+1]}.png"
        pyautogui.screenshot(path)
        print("SHOT", path)
        i += 2
    elif c in ("click", "rclick"):
        sx, sy = game_to_screen(float(cmds[i+1]), float(cmds[i+2]))
        pyautogui.moveTo(sx, sy, duration=0.15)
        pyautogui.click(button="right" if c == "rclick" else "left")
        print(c.upper(), sx, sy)
        i += 3
    elif c == "key":
        pyautogui.press(cmds[i+1])
        i += 2
    elif c == "type":
        pyautogui.typewrite(cmds[i+1], interval=0.03)
        i += 2
    elif c == "wait":
        time.sleep(float(cmds[i+1]))
        i += 2
    else:
        print("unknown", c); break
