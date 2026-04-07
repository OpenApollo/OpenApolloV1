#Demo code for taking sensor quaternion data and rotating servos with it
#Also sends data to HC12
#Make sure to set values according to your servo
#This is a very slow loop, but that's because it's in python
# the STM sends orientation data at several Hz and can go even faster.
# so the huge bottleneck here is most likely python

import serial
from time import sleep, time
import re
import math

import board
import busio
from adafruit_pca9685 import PCA9685

ser = serial.Serial("/dev/ttyAMA3", 230400)
serialRadio = serial.Serial("/dev/ttyAMA4", 9600)
buffer = ""

i2c = busio.I2C(board.SCL, board.SDA)
pca = PCA9685(i2c)
pca.frequency = 50

#Servo values:
NEUTRAL_US = 1540
FULL_FORWARD_US = 2400
FULL_REVERSE_US = 450

# Your servo speed:
SERVO_SPEED_DEG_PER_SEC = 150.0

# Ignore tiny noise
DEADZONE_DEG = 0.05

# Channels 0 to 3
CHANNELS = [0, 1, 2, 3]

# Store previous quaternion
prev_q = None

# Store active motion end times for each servo channel
move_until = [0.0, 0.0, 0.0, 0.0]

def us_to_duty_cycle(us, frequency=50):
    period_us = 1_000_000 / frequency
    duty = int((us / period_us) * 65535)
    return max(0, min(65535, duty))

def write_servo_us(channel, pulse_us):
    pca.channels[channel].duty_cycle = us_to_duty_cycle(pulse_us)

def stop_servo(channel):
    write_servo_us(channel, NEUTRAL_US)

def run_servo(channel, direction, speed_scale=1.0):
    """
    direction: +1 or -1
    speed_scale: 0.0 to 1.0
    """
    speed_scale = max(0.0, min(1.0, speed_scale))

    if direction >= 0:
        pulse = NEUTRAL_US + (FULL_FORWARD_US - NEUTRAL_US) * speed_scale
    else:
        pulse = NEUTRAL_US - (NEUTRAL_US - FULL_REVERSE_US) * speed_scale

    write_servo_us(channel, int(pulse))

def extract_quaternion(msg: str):
    try:
        i1 = msg.index("#quatI:") + 7
        i2 = msg.index("#quatJ:")
        qi = int(msg[i1:i2]) * (1.0 / 16384.0)

        j1 = i2 + 7
        j2 = msg.index("#quatK:")
        qj = int(msg[j1:j2]) * (1.0 / 16384.0)

        k1 = j2 + 7
        k2 = msg.index("#quatW:")
        qk = int(msg[k1:k2]) * (1.0 / 16384.0)

        w1 = k2 + 7
        qw = int(msg[w1:].strip()) * (1.0 / 16384.0)

        return qi, qj, qk, qw

    except(ValueError, IndexError):
        return None

def quat_conjugate(q):
    x, y, z, w = q
    return (-x, -y, -z, w)

def quat_multiply(q1, q2):
    x1, y1, z1, w1 = q1
    x2, y2, z2, w2 = q2

    return (
        w1*x2 + x1*w2 + y1*z2 - z1*y2,
        w1*y2 - x1*z2 + y1*w2 + z1*x2,
        w1*z2 + x1*y2 - y1*x2 + z1*w2,
        w1*w2 - x1*x2 - y1*y2 - z1*z2
    )

def quat_to_euler(q):
    x, y, z, w = q

    # roll
    sinr = 2 * (w*x + y*z)
    cosr = 1 - 2 * (x*x + y*y)
    roll = math.atan2(sinr, cosr)

    # pitch
    sinp = 2 * (w*y - z*x)
    sinp = max(-1.0, min(1.0, sinp))
    pitch = math.asin(sinp)

    # yaw
    siny = 2 * (w*z + x*y)
    cosy = 1 - 2 * (y*y + z*z)
    yaw = math.atan2(siny, cosy)

    return roll, pitch, yaw

def start_timed_correction(channel, angle_deg):
    """
    For a continuous-rotation servo:
    - angle_deg tells how much motion we want to counter
    - we drive at full speed in the needed direction
    - we stop after the computed time
    """
    global move_until

    if abs(angle_deg) < DEADZONE_DEG:
        stop_servo(channel)
        move_until[channel] = 0.0
        return

    direction = 1 if angle_deg > 0 else -1
    duration = abs(angle_deg) / SERVO_SPEED_DEG_PER_SEC

    # Start moving at full speed
    run_servo(channel, direction, speed_scale=1.0)

    # Schedule a stop
    move_until[channel] = time() + duration

#Main loop
print("Running quaternion-to-timed-servo control...")

# Start all servos stopped
for ch in CHANNELS:
    stop_servo(ch)

count = 0
try:
    while True:
        now = time()

        # Stop any servos whose motion time has elapsed
        for ch in CHANNELS:
            if move_until[ch] != 0.0 and now >= move_until[ch]:
                stop_servo(ch)
                move_until[ch] = 0.0

        # Read serial data
        if ser.in_waiting > 0:
            sleep(0.001)
            #data = ser.read(ser.in_waiting)
            data = ser.readline()
            buffer += data.decode("utf-8", errors="ignore")
            count+=1
            if(count >= 2):
                serialRadio.write(data)
                count = 0

            q = extract_quaternion(buffer)
            if q is None:
                print("q is none")
                continue

            buffer = ""

            # First quaternion is just reference
            if prev_q is None:
                prev_q = q
                print("Reference quaternion stored.")
                continue

            # Relative rotation since last sample
            q_delta = quat_multiply(q, quat_conjugate(prev_q))
            prev_q = q

            d_roll, d_pitch, d_yaw = quat_to_euler(q_delta)

            # Convert radians to degrees
            d_roll = math.degrees(d_roll)
            d_pitch = math.degrees(d_pitch)
            d_yaw = math.degrees(d_yaw)

            # Small noise suppression
            if abs(d_roll) < DEADZONE_DEG:
                d_roll = 0.0
            if abs(d_pitch) < DEADZONE_DEG:
                d_pitch = 0.0
            if abs(d_yaw) < DEADZONE_DEG:
                d_yaw = 0.0

            # Map quaternion change to servo corrections
            # Flip signs if the correction moves the wrong way.
            corrections = [
                -d_roll,   # channel 0
                -d_pitch,  # channel 1
                -d_yaw,    # channel 2
                d_roll     # channel 3
            ]

            for ch, angle_deg in zip(CHANNELS, corrections):
                start_timed_correction(ch, angle_deg)

            print(f"dR={d_roll:.2f}°, dP={d_pitch:.2f}°, dY={d_yaw:.2f}°")

except KeyboardInterrupt:
    pass
finally:
    for ch in CHANNELS:
        stop_servo(ch)
    pca.deinit()
    ser.close()
    print("Stopped.")
