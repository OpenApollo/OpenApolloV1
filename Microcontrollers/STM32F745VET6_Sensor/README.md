Basic settings for this to work (STM32F745VET6):

Sys: Serial Wire
I2C1: it's a 100 kHz here, I know the BNO can handle 400, but I'm not sure about the barometer
UART4: 9600 baud (GPS)
USART2: 230400 baud (CM5)
USART3: 57600 baud (PIC)
PC4 (32) pin: GPIO_Input, optional name of "BNO_FLAG", no pull-up or pull-down
  ^The BNO085's Interrupt pin has to be connected to this via jumper or solder. Design flaw here, will fix in V2. Sry

The code I added here is using the internal oscillator, but setting an external one isn't difficult. OSC_IN and OSC_OUT
and make sure to route the clock signal correctly on the second tab. 16MHz recommended for external oscillator
