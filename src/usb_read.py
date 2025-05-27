from evdev import InputDevice, KeyEvent, categorize, ecodes, list_devices
from typing import cast
import subprocess
import serial

# setup the evdev
inp_path = "/dev/input/event17"

print("Give permissions to read the file")
subprocess.run(["sudo", "chmod", "a+r", inp_path])

kbd = InputDevice(inp_path)
print(f"Listening to {kbd.name}...")

# evdev keymaps to Arduino Keyboard codes
key_map = {
    ecodes.KEY_A: ord("a"),
    ecodes.KEY_B: ord("b"),
    ecodes.KEY_C: ord("c"),
    ecodes.KEY_D: ord("d"),
    ecodes.KEY_E: ord("e"),
    ecodes.KEY_F: ord("f"),
    ecodes.KEY_G: ord("g"),
    ecodes.KEY_H: ord("h"),
    ecodes.KEY_I: ord("i"),
    ecodes.KEY_J: ord("j"),
    ecodes.KEY_K: ord("k"),
    ecodes.KEY_L: ord("l"),
    ecodes.KEY_M: ord("m"),
    ecodes.KEY_N: ord("n"),
    ecodes.KEY_O: ord("o"),
    ecodes.KEY_P: ord("p"),
    ecodes.KEY_Q: ord("q"),
    ecodes.KEY_R: ord("r"),
    ecodes.KEY_S: ord("s"),
    ecodes.KEY_T: ord("t"),
    ecodes.KEY_U: ord("u"),
    ecodes.KEY_V: ord("v"),
    ecodes.KEY_W: ord("w"),
    ecodes.KEY_X: ord("x"),
    ecodes.KEY_Y: ord("y"),
    ecodes.KEY_Z: ord("z"),
    ecodes.KEY_1: ord("1"),
    ecodes.KEY_2: ord("2"),
    ecodes.KEY_3: ord("3"),
    ecodes.KEY_4: ord("4"),
    ecodes.KEY_5: ord("5"),
    ecodes.KEY_6: ord("6"),
    ecodes.KEY_7: ord("7"),
    ecodes.KEY_8: ord("8"),
    ecodes.KEY_9: ord("9"),
    ecodes.KEY_0: ord("0"),
    ecodes.KEY_LEFTCTRL: 128,
    ecodes.KEY_LEFTSHIFT: 129,
    ecodes.KEY_ENTER: 176,
    ecodes.KEY_SPACE: ord(" "),
    ecodes.KEY_BACKSPACE: 178,
}

# initialize the serial connection
ser = serial.Serial('/dev/ttyUSB0', 115200)  # or 'COM3' on Windows

for event in kbd.read_loop():
    if event.type == ecodes.EV_KEY:
        key_event = cast(KeyEvent, categorize(event))
        key = key_map.get(key_event.scancode, ord("?"))
        if key_event.keystate == KeyEvent.key_down:
            print(f"DOWN: {key}")
            ser.write('1'.encode())
            ser.write(bytes([key]))
        elif key_event.keystate == KeyEvent.key_up:
            print(f"UP: {key}")
            ser.write('0'.encode())
            ser.write(bytes([key]))

