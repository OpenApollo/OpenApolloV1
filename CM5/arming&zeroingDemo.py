#Sample code. Waits to receive data from HC12
#if receiving "ARMWS\x00", it arms and zeros servos
#if receiving "ARMNS\x00", it arms without zeroing servos
#The code here only zeros servo on channel 0
#Update the servo definitions to work for you
#Arming means it allows the STM32 to start reading and transmitting sensor data
# otherwise it just sits there without reading sensor data

#A lot of confusing, unneccessary, or redundant code here
#but it works so I'm not touching it

import serial
from time import sleep
import board
import busio
from adafruit_pca9685 import PCA9685

serialSensor = serial.Serial("/dev/ttyAMA3", 230400)
serialRadio  = serial.Serial("/dev/ttyAMA4", 9600)

#Servo definitions:
THRESHOLD = 480
NEUTRAL_US = 1540
RUN_US = 2000
SERVO_CH = 0

i2c = busio.I2C(board.SCL, board.SDA)
pca = PCA9685(i2c)
pca.frequency = 50

print("Started")

def us_to_duty(us):
    return int(us / 20000 * 65535)

def set_servo(channel, us):
    pca.channels[channel].duty_cycle = us_to_duty(us)

def armingSetup(zeroServos):
    print("ARMING STM32")
    for c in range (10):
        serialSensor.write(b"ARM")
        sleep(0.2)
    sleep(0.01)
    if serialSensor.in_waiting > 0:
        sleep(0.5)
        receivedSensor = serialSensor.read(serialSensor.in_waiting)
        print(receivedSensor.decode("utf-8", errors="ignore"))
        print("STM32 ARMED")
    sleep(1)
    serialSensor.read(serialSensor.in_waiting)
    if(zeroServos):
        print("Zeroing servos")
        serialSensor.write(b"POS")
        sleep(3)
        serialSensor.write(b"POS")
        setServo()
        for c in range(5):
            serialSensor.write(b"STP")
            sleep(0.5)
    else:
        print("Skipping servo zeroing")
        for c in range (5):
            if(zeroQ):
                serialSensor.write(b"NEG")
                sleep(0.2)


                    


def singleReadSensor():
    print("singleReadSensor:")
    if serialSensor.in_waiting > 0:
        sleep(0.2)
        receivedSensor = serialSensor.read(serialSensor.in_waiting)
        decodeSensor = receivedSensor.decode("utf-8", errors="ignore")
        print(receivedSensor.decode("utf-8", errors="ignore"))

def readSensor():
    count = 0
    while(1):
        if serialSensor.in_waiting > 0:
            sleep(0.2)
            receivedSensor = serialSensor.read(serialSensor.in_waiting)
            print(receivedSensor.decode("utf-8", errors="ignore"))
            count+=1
            if(count >= 2):
                serialRadio.write(receivedSensor)
                count = 0


def setServo():
    print("Starting servo...")
    set_servo(SERVO_CH, RUN_US)
    sleep(0.5)  # Give servo time to initialise before we start reading

    buffer   = ""
    running  = True

    try:
        while running:
            if serialSensor.in_waiting > 0:
                data  = serialSensor.read(serialSensor.in_waiting)
                chunk = data.decode("utf-8", errors="ignore")
                buffer += chunk

            # Split on newlines; keep last partial line in buffer
            lines  = buffer.split("\n")
            buffer = lines[-1]

            for line in lines[:-1]:
                line = line.strip()
                if not line:
                    continue

                print("LINE:", line)   # remove when stable

                if line.startswith("#A00:"):
                    try:
                        value = int(line.split(":")[1])
                        print(f"A00 = {value}")
                        if value > THRESHOLD:
                            print(">>> STOP CONDITION MET <<<")
                            set_servo(SERVO_CH, NEUTRAL_US)
                            running = False
                            break          # exit the for-loop; while sees running=False
                    except ValueError:
                        print("Parse error on line:", line)

    except KeyboardInterrupt:
        print("Interrupted by user.")

    finally:
        set_servo(SERVO_CH, NEUTRAL_US)
        sleep(0.1)
        print("Stopped safely.")

def radioListen():
    while(1):
        if serialRadio.in_waiting > 0:
            print("inwaiting")
            receivedRadio = serialRadio.read(serialRadio.in_waiting)
            decodeRadio = receivedRadio.decode("utf-8", errors="ignore")
            #decodeRadio = decodeRadio.strip() #Because it gives ARMWS\x00
            print("decodeRadio = ",repr(decodeRadio))
            print(decodeRadio == "ARMWS\x00")
            if(decodeRadio == "ARMWS\x00"):
                print("ARMING WITH SERVO")
                zeroServos = True
                armingSetup(zeroServos)
                readSensor()
            elif(decodeRadio == "ARMNS\x00"):
                zeroServos = False
                armingSetup(zeroServos)
                readSensor()

radioListen()








