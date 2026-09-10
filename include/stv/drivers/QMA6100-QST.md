Document #:  13-52-21  Title:  QMA6100 Datasheet

Rev: A

矽睿

Abstract
3-Axis Accelerometer QMA6100

Advanced Information

The QMA6100 is a 3-Axis Accelerometer. This surface-mount, small sized chip has integrated acceleration transducer

with signal conditioning ASIC, sensing tilt, motion, shock and vibration, targeted for applications such as screen rotation,
step counting, sleep monitor, gaming and personal navigation in mobile and wearable smart devices.

The  QMA6100  is based on  the state-of-the-art, high  resolution single crystal silicon MEMS technology. Along with
custom-designed 14-bit ADC ASIC, it offers the advantages of low noise, high accuracy, low power consumption, and offset
trimming. The device supports digital interface I2C and SPI.

The QMA6100 is in a 2x2x0.95 mm3 surface mount 12-pin land grid array (LGA) package.

                            FEATURES                                                  BENEFIT

  3-Axis Accelerometer in a 2x2x0.95 mm3 Land
Grid  Array  Package  (LGA),  guaranteed  to
operate over a temperature range of -40 °C to
+85 °C.

    Small size for highly integrated products.

Signals have been digitized and factory
trimmed.

  14-Bit  ADC  with  low  noise  accelerometer

    High resolution allows for motion and tilt

sensor

  I2C Interface with SDR modes.
Support SPI digital interface

  Built-In Self-Test

  Wide range operation voltage (1.71V to 3.6V)
and  low  power  consumption  (2-50uA  low
power conversion current)

sensing

    High-Speed Interfaces for fast data

communications.

    Enables low-cost functionality test after

assembly in production

    Automatically maintains sensor’s sensitivity

under wide operation voltage range and
compatible with battery powered
applications

  Integrated FIFO with depth of 64 frames

    Environmental protection and wide

RoHS compliant, halogen-free

applications

  Built–in motion algorithm

    Low power and easy applications including
step counting, sleep monitor, gaming and
personal navigation

Document #:  13-52-21  Title:  QMA6100 Datasheet

Rev: A

矽睿

CONTENTS

5

2

3.1
3.2

CONTENTS ....................................................................................................................................................................................... 2
INTERNAL SCHEMATIC DIAGRAM ................................................................................................................................... 3
1
1.1
Internal Schematic Diagram ..................................................................................................................................... 3
SPECIFICATIONS AND I/O CHARACTERISTICS .............................................................................................................. 3
Product Specifications .............................................................................................................................................. 3
2.1
Absolute Maximum Ratings ..................................................................................................................................... 4
2.2
I/O Characteristics .................................................................................................................................................... 5
2.3
3    PACKAGE PIN CONFIGURATIONS ..................................................................................................................................... 5
Package 3-D View .................................................................................................................................................... 5
Package Outlines ...................................................................................................................................................... 7
4    EXTERNAL CONNECTION ................................................................................................................................................... 8
I2C Dual Supply Connection .................................................................................................................................... 8
4.1
I2C Single Supply connection .................................................................................................................................. 9
4.2
SPI Dual Supply Connection .................................................................................................................................... 9
4.3
4.4
SPI Single Supply connection ................................................................................................................................ 10
BASIC DEVICE OPERATION .............................................................................................................................................. 11
Acceleration sensor................................................................................................................................................. 11
5.1
Power Management ................................................................................................................................................ 11
5.2
Power On/Off Time ................................................................................................................................................ 12
5.3
Communication Bus Interface I2C and Its Addresses ............................................................................................. 13
5.4
6    MODES OF OPERATION ..................................................................................................................................................... 13
Modes Transition .................................................................................................................................................... 13
6.1
6.2
Description of Modes ............................................................................................................................................. 14
Functions and interrupts .......................................................................................................................................................... 14
STEP_ INT ............................................................................................................................................................. 15
7.1
DRDY_INT ............................................................................................................................................................ 15
7.2
ANY_MOT_INT .................................................................................................................................................... 16
7.3
SIG_MOT_INT ...................................................................................................................................................... 18
7.4
NO_MOT_INT ....................................................................................................................................................... 18
7.5
TAP_INT ................................................................................................................................................................ 19
7.6
RAISE_INT ............................................................................................................................................................ 20
7.7
FIFO_INT ............................................................................................................................................................... 20
7.8
7.9
Interrupt configuration ............................................................................................................................................ 21
  DIGITAL INTERFACE ........................................................................................................................................................ 23
I2C Timings ............................................................................................................................................................ 23
8.1
I2C R/W Operation ................................................................................................................................................. 23
8.2
Serial Peripheral Interface(SPI) .............................................................................................................................. 25
8.3
9    REGISTERS ........................................................................................................................................................................... 28
Register Map .......................................................................................................................................................... 28
Register Definition ................................................................................................................................................. 30

9.1
9.2

8

7

The information contained herein is the exclusive property of QST, and shall not be distributed,
reproduced, or disclosed in whole or in part without prior written permission of QST.

  2  /  40

Document #:  13-52-21  Title:  QMA6100 Datasheet

Rev: A

矽睿

1

INTERNAL SCHEMATIC DIAGRAM

1.1  Internal Schematic Diagram

Figure 1.    Block Diagram

Table 1.    Block Function

Block
Transducer
CVA
Interrupt

FSM
I2C/SPI
OSC
Power

Function
3-axis acceleration sensor
Charge-to-Voltage amplifier for sensor signals
Digital interrupt engine, to generate interrupt signal on data conversion, and
motion function
Finite state machine, to control device in different mode
Interface logic data I/O
Oscillator for internal operation
Power block, including LDO

The information contained herein is the exclusive property of QST, and shall not be distributed,
reproduced, or disclosed in whole or in part without prior written permission of QST.

  3  /  40

                     G-SENSOROSCPOWER(A+D)SCXSDXINT1VDDGNDAD0BGSDMVPMXYZGNDOffsetGainModeFSInterruptIF(I2C/SPI)FSMOTPReg FileFIFOSincSelfTestPORCVAINT2SENBRESV2

Document #:  13-52-21  Title:  QMA6100 Datasheet

Rev: A

矽睿

2  SPECIFICATIONS AND I/O CHARACTERISTICS

2.1

Product Specifications

Table 2.    Specifications (* Tested and specified at 25°C and 3.0V VDD except stated otherwise.)

Parameter
Supply voltage VDD
I/O voltage VDDIO
Standby current

Low power current

Conditions
VDD, for internal blocks
VDDIO, for IO only
VDD and VDDIO on
ODR=800 Hz
ODR=400 Hz
ODR=160 Hz
ODR=80Hz

Low noise current

ODR=50 Hz
ODR=25 Hz
ODR=12.5

Data output rate
(ODR)

Startup time

Wakeup time

Operating
temperature
Acceleration Full
Range

Sensitivity

FS=±2g
FS=±4g
FS=±8g
FS=±16g
FS=±32g

Sensitivity
Temperature Drift
Sensitivity tolerance
Zero-g offset
Zero-g offset
Temperature Drift
Noise density
Nonlinearity
Cross Axis Sensitivity

FS=±2g, Normal VDD Supplies

Gain accuracy
FS=±2g, Normal VDD Supplies

FS=±2g, Normal VDD Supplies

FS=±2g, run state
FS=±2g, Best fit straight line,

Min
1.71
1.71

Typ
3.0
3.0
1
34
17
7
4

134
67
34

Max
3.6
VDD

Unit
V
V
μA

μA

μA

1.25

1000

Samples
/sec

ms

ms

℃

g

LSB/g

%/℃

%
mg

mg/℃

μg/√Hz
%FS
%

±2/±4/±8/
±16/±32
4096
2048
1024
512
256

±0.02

±4
±80

±2

220
±0.5
1

From the time when VDD reaches to
90% of final value to the time when
device is ready for conversion

From the time device enters into
active mode to the time device is
ready for conversion

2

1

-40

85

The information contained herein is the exclusive property of QST, and shall not be distributed,
reproduced, or disclosed in whole or in part without prior written permission of QST.

  4  /  40

Document #:  13-52-21  Title:  QMA6100 Datasheet

Rev: A

矽睿

2.2

Absolute Maximum Ratings

Table 3.    Absolute Maximum Ratings (Tested at 25°C except stated otherwise.)
Max
Item
Power Supply Voltage
5.4
Input Voltage (other than power)
Reflow Classification
Storage Temperature
Storage Humidity
ESD(HBM)
ESD(MM)
ESD(CDM)
Shock Immunity

150
95
±2000
±200
±500
10000

Tstr
Hstr
Vhbm
Vmm
Vcdm

-50
10

Symbol  Min
-0.3
Vddmax
Vmax
-0.2  Vdd+0.2
MSL3,260℃ Peak Temperature

V
V

℃
%RH

V
V
V
g

duration < 200uS

Unit  Remark

2.3  I/O Characteristics

Table 4.    I/O Characteristics

Item

Digital Input Low Voltage
Digital Input High Voltage
Digital Input Hysterisis
Digital Output Low Voltage(I2C)
Digital Output Low Voltage (SPI)
Digital Output High Voltage1
(SPI)
(Vio>=1.62V)
Digital Output High Voltage2
(SPI)
(Vio>=1.2V)
Leakage Current at Output OFF

Internal Pullup Resistor
I2C Load Capacitor
Load Capacitance of
Reset Terminal
Pulse Width of
Asynchronous Reset
Power on Startup Time

Symbol
Vil_d
Vih_d
Vidhys
Vol_d1
Vol_d2

Voh_d1

Voh_d2

Ioff

Rpullup

Cb

Crst

Trst

Tstart

3    PACKAGE PIN CONFIGURATIONS

3.1  Package 3-D View

Condition

Io=3mA (SDI) *1)
Io=1mA (SDI, SDO) *1)

Io=1mA (SDI, SDO) *1)

Io=1mA (SDI, SDO) *1)
SDX, AD0

SENB
SDX, SCX

Min
-
Vddio*0.8
Vddio*0.1
0
0

Vddio*0.8

Vddio*0.6

-10

70

-

-

100

-

Typ
-
-
-
-
-

Max
Vddio*0.2
-
-
Vddio*0.2
Vddio*0.2

-

-

-

120

-

-

-

-

-

-

10

190

400

20

-

10

Unit
V
V
V
V
V

V

V

µA
koh
m
pF

pF

µsec

msec

Arrow indicates direction of g field that generates a positive output reading in normal measurement configuration.

The information contained herein is the exclusive property of QST, and shall not be distributed,
reproduced, or disclosed in whole or in part without prior written permission of QST.

  5  /  40

Document #:  13-52-21  Title:  QMA6100 Datasheet

Rev: A

矽睿

Figure 2.    Package View

Table 5.    Pin Configurations

No  Name

IO  Description

Logic Level

I

AD0
SDX
VDDIO
RESV1
INT1
INT2
VDD

Power supply to IO
Reserved
Interrupt1
Interrupt2
Power supply to internal circuitry

LSB of I2C address, or SDO of SPI serial data output
1
I/O  SDA of I2C serial data, or SDI of SPI serial data input
2
P
3
A
4
O
5
O
6
7
P
8  GNDIO  G  Ground to IO
9
10
11
12

Protocol selection
Reserved
SCL of I2C serial clock, or SCK of SPI serial clock

G  Ground to internal circuitry
I
A
I

GND
SENB
RESV2
SCX

VDDIO
VDDIO
VDDIO
VDDIO
VDDIO
VDDIO
VDD
GND
GND
VDDIO
VDDIO
VDDIO

No  Name

IO

1

AD0

I

I2C
VDDIO/GND

Connectivity
SPI_3W
Float

SPI_4W
MISO

The information contained herein is the exclusive property of QST, and shall not be distributed,
reproduced, or disclosed in whole or in part without prior written permission of QST.

  6  /  40

AD0SCXRESV2SENBRESV1INT1INT2VDDVDDIOSDXGNDIOGND14561211710QMA6100Top View2389xy

Document #:  13-52-21  Title:  QMA6100 Datasheet

Rev: A

矽睿

I/O
SDX
2
P
VDDIO
3
A
RESV1
4
O
INT1
5
O
INT2
6
7
P
VDD
8  GNDIO  G
G
GND
9
I
SENB
10
A
RESV2
11
I
SCX
12

SDA
VDDIO
Float/GND
INT1
INT2
VDD
GND
GND
VDDIO/Float
VDDIO/Float/GND
SCL

SDI/SDO
VDDIO
Float/GND
INT1
INT2
VDD
GND
GND
CSB
VDDIO/Float/GND
SCK

MOSI
VDDIO
Float/GND
INT1
INT2
VDD
GND
GND
CSB
VDDIO/Float/GND
SCK

3.2  Package Outlines

3.2.1

Package Type

LGA (Land Grid Array)

3.2.2

Package Outline Drawing:

2.0mm (Length)*2.0mm (Width)*0.95mm (Height)

NOTE:
1.  CONTROLLING DIMENSION: MILLIMETER.
Figure 3.    Package Outline Drawing

3.2.3

Tape And Reel

Devices are shipped in reels, in standard cardboard box packaging.

Package

Reel Size  WidthxPitch

Qty/reel

Trailer(Inner

Leader(Outer

Pin 1 Location

The information contained herein is the exclusive property of QST, and shall not be distributed,
reproduced, or disclosed in whole or in part without prior written permission of QST.

  7  /  40

Document #:  13-52-21  Title:  QMA6100 Datasheet

Rev: A

矽睿

LGA(2x2)

13”

12*4

5000

layer Min length)
300mm

layer Min length
300mm

Up Right

4    EXTERNAL CONNECTION

4.1

I2C Dual Supply Connection

Figure 5.      I2C Dual Supply Connection

The information contained herein is the exclusive property of QST, and shall not be distributed,
reproduced, or disclosed in whole or in part without prior written permission of QST.

  8  /  40

AD0SCXRESV2SENBRESV1INT1INT2VDDVDDIOSDXGNDIOGNDTop ViewQMA6100Digital Power1.71~VDDPowerSCLSDAInterruptC3, 10nFC1, 0.1uFC2, 2.2uF2.2kohm2.2kohmAnalog Power1.71~3.6V

Document #:  13-52-21  Title:  QMA6100 Datasheet

Rev: A

矽睿

4.2

I2C Single Supply connection

Figure 6.    I2C Single Supply Connection

4.3

SPI Dual Supply Connection

The information contained herein is the exclusive property of QST, and shall not be distributed,
reproduced, or disclosed in whole or in part without prior written permission of QST.

  9  /  40

AD0SCXRESV2SENBRESV1INT1INT2VDDVDDIOSDXGNDIOGNDTop ViewQMA6100Power1.71~3.6VPowerSCLSDAInterruptC3, 10nFC1, 0.1uFC2, 2.2uF2.2kohm2.2kohm

Document #:  13-52-21  Title:  QMA6100 Datasheet

Rev: A

矽睿

Figure 7.      SPI Dual Supply Connection

4.4

SPI Single Supply connection

The information contained herein is the exclusive property of QST, and shall not be distributed,
reproduced, or disclosed in whole or in part without prior written permission of QST.

  10  /  40

AD0SCXRESV2SENBRESV1INT1INT2VDDVDDIOSDXGNDIOGNDTop ViewQMA6100Digital Power1.71~VDDPowerSCKSDOInterruptAnalog Power1.71~3.6VC3, 10nFC1, 0.1uFC2, 2.2uFSDICSB

Document #:  13-52-21  Title:  QMA6100 Datasheet

Rev: A

矽睿

Figure 8.    SPI Single Supply Connection

5  BASIC DEVICE OPERATION

5.1

Acceleration sensor

The QMA6100 acceleration sensor circuit consists of tri-axial sensors and application specific support circuits to measure the
acceleration of device. When a DC power supply is applied to the sensor, the sensor converts any accelerating incident in the
sensitive axis directions to charge output.

5.2

Power Management

Device has two power supply pins. VDD is the main power supply for all of the internal blocks, including analog and digital.
VDDIO is a separate power supply, for digital interface only.

The device contains a power-on-reset generator. It generates reset pulse as power on, which can load the register’s default
value, for the device to function properly.
To make sure the POR block functions well, we should have such constrains on the timing of VDD and VDDIO.

The information contained herein is the exclusive property of QST, and shall not be distributed,
reproduced, or disclosed in whole or in part without prior written permission of QST.

  11  /  40

AD0SCXRESV2SENBRESV1INT1INT2VDDVDDIOSDXGNDIOGNDTop ViewQMA6100Power1.71~3.6VPowerSCKSDOInterruptC3, 10nFC1, 0.1uFC2, 2.2uFSDICSB

Document #:  13-52-21  Title:  QMA6100 Datasheet

Rev: A

矽睿

The device should turn-on both power pins in order to operate properly. When the device is powered on, all registers are
reset by POR, then the device transits to the standby mode and waits for further commends.

Table 6 provides references for four power states.

Table 6．  Power States

Power State
1
2

VDD
0V
0V

VDDIO
0V
1.71V-3.6V

3
4

1.71V-3.6V
1.71V-3.6V

0V
1.71V-VDD

Power State Description
Device off
Not allowed. User need to make sure VDDIO is less than
VDD
Device off
Device on, normal operation mode, enters standby mode
after POR

5.3

Power On/Off Time

Device has two power supply pins and two ground pins. VDD is the main power supply for all of the internal blocks,
including analog and digital. VDDIO is a separate power supply, for digital interface only. GND is 0V supply for all of internal
blocks, and GNDIO for digital interface.

There is no limitation on the voltage levels of VDD and VDDIO relative to each other, as long as they are within operating
range.

The device contains a power-on-reset generator. It generates reset pulse as power on, which can load the register’s
default value, for the device to function properly.

To make sure the POR block functions well, we should have such constrains on the timing of VDD.

                The power on/off time related to the device is in Table 7

Table 7. Time Required for Power On/Off

Parameter
POR Completion
Time

Symbol
PORT

Power off Voltage

SDV

Power on Interval

PINT

Power on Time

PSUP

Condition
Time Period After VDD and
VDDIO at Operating Voltage to
Ready for I2C Commend and
Analogy Measurement.
Voltage that Device Considers to
be Power Down.
Time Period Required for Voltage
Lower Than SDV to Enable Next
POR
Time Period Required for Voltage
from SDV to 90% of final value

Min.

Typ.

Max.
250

Unit
μs

100

0.2

V

μs

50

ms

The information contained herein is the exclusive property of QST, and shall not be distributed,
reproduced, or disclosed in whole or in part without prior written permission of QST.

  12  /  40

Document #:  13-52-21  Title:  QMA6100 Datasheet

Rev: A

矽睿

Figure 9.    Power On/Off Timing

5.4

Communication Bus Interface I2C and Its Addresses

This device will be connected to a serial interface bus as a slave device under the control of a master device, such as the
processor. Control of this device is carried out via I²C.

This device is compliant with I²C -Bus Specification, document number:    9398 393 40011.    As an I²C compatible device, this
device has a 7-bit serial address and supports I²C protocols.    This device supports standard and fast speed modes, 100 kHz
and 400 kHz, respectively. External pull-up resistors are required to support all these modes.

There are two I2C addresses selected by connecting pin 1 (AD0) to GND or VDD.    The first six MSB are hardware configured
to “001001” and the LSB can be configured by AD0.

Table 8.    I2C Address Options

AD0 (pin 1)
Connect to GND
Connect to VDD

I2C Slave Address (HEX)
12
13

I2C Slave Address (BIN)
0010010
0010011

6    MODES OF OPERATION

6.1    Modes Transition

QMA6100 has two different operational modes, controlled by register (0x11), MODE_BIT. The main purpose of these modes
is for power management. The modes can be transited from one to another, as shown below, through I2C commands. The
default mode after power-on is standby mode.

The information contained herein is the exclusive property of QST, and shall not be distributed,
reproduced, or disclosed in whole or in part without prior written permission of QST.

  13  /  40

Document #:  13-52-21  Title:  QMA6100 Datasheet

Rev: A

矽睿

Figure 10.    Basic operation flow after power-on                            Figure 11.    The work mode transferring

The default mode after power on is standby mode. Through I2C instruction, device can switch between standby mode and
active mode. With SOFTRESET by writing 0xB6 into register 0x36, all of the registers will get default values. SOFTRESET can
be done both in active mode and in standby mode. Also, by writing 1 in NVM_LOAD (0x33<3>) when device is in active
mode, the NVM related image registers will get default value from NVM, however, other registers will keep the values of
their own.

6.2  Description of Modes

6.2.1

Active Mode

In active mode, the ADC digitizes the charge signals from transducer, and digital signal processor conditions these signals in
digital domain, processes the interrupts, and send data to Data registers (0x01~0x06) and FIFO (accessible through register
0x3F).

6.2.2

Standby Mode

In standby mode, most of the blocks are off, while device is ready for access through I2C. Standby mode is the default mode
after power on or soft reset. Device can enter into this mode by set the soft reset register (0x36) to 0xB6 or set the
MODE_BIT (0x11<7>) to logic 0.

7  Functions and interrupts

ASIC support interrupts, such as STEP_INT, DRDY_INT, ANY_MOT_INT, SIG_MOT_INT, NO_MOT_INT, RAISE_INT and
FIFO_INT, etc.

The information contained herein is the exclusive property of QST, and shall not be distributed,
reproduced, or disclosed in whole or in part without prior written permission of QST.

  14  /  40

Power OffReset(POR or Soft Reset)NVM LoadStandbyActive0x11<7>=10x11<7>=00x33<3>=10x36=0xB60x36=0xB6Reset(POR or Soft Reset)NVM LoadStandbyActive0x11<7>=10x11<7>=00x33<3>=10x36=0xB60x36=0xB6

Document #:  13-52-21  Title:  QMA6100 Datasheet

Rev: A

矽睿

7.1  STEP_ INT

The STEP_FPAG detect that the user is entering/exiting step mode. When the user enters into step mode, at least one axis
sensor data will vary periodically, by numbering the variation periods and the acceleration intensity the step counter can be
calculated.

Figure 10.    STEP SIGNAL

The related interrupt status bit is STEP_INT (0x0A<3>) and SIG_STEP (0x0A<6>). When the interrupt is generated, the value
of STEP_INT will be set to logic 1, which will be cleared after the interrupt status register is read by user.
STEP_IEN/SIG_STEP_IEN (0x16<3>/0x16<6>) is the enable bit for the STEP_INT/SIG_STEP_INT. Also, to get this interrupt on
PIN_INT1 and/or PIN_INT2, we need to set INT1_STEP (0x19<3>)/INT1_SIG_STEP (0x19<6>)      or INT2_STEP (0x1B<3>)
/INT2_SIG_STEP (0x1B<6>) to logic 1, to map the internal interrupt to the interrupt PINs.

7.2  DRDY_INT

The width of the acceleration data is 14 bits, in two’s complement representation. The data of each axis is split into 2 parts,
the MSB part (one byte contains bit 13 to bit 6) and the LSB part (one byte contains bit 5 to bit 0). Reading data should start
with LSB part. When user is reading the LSB byte of data, to ensure the integrity of the acceleration data, the content of
MSB can be locked, by setting SHADOW_DIS (0x21<6>) to logic 0. This lock function can be disabled by setting SHADOW_DIS
to logic 1. Without lock, the MSB and LSB content will be updated by new value immediately. The bit NEW_DATA in the LSB
byte is the flag of the new data. If new data is updated, this NEW_DATA flag will be 1, and will be cleared when
corresponding MSB or LSB is read by user.

Also, the user should note that even with SHADOW_DIS=0, the data of 3 axes are not guaranteed from the same time point.

The device supports four different acceleration measurement ranges. The range is setting through RANGE (0x0F<3:0>), and
the details as following:

RANGE

0001
0010
0100

Acceleration
range
2g
4g
8g

Resolution

244ug/LSB
488ug/LSB
977ug/LSB

The information contained herein is the exclusive property of QST, and shall not be distributed,
reproduced, or disclosed in whole or in part without prior written permission of QST.

  15  /  40

Document #:  13-52-21  Title:  QMA6100 Datasheet

Rev: A

16g
32g
2g

1.95mg/LSB
3.91mg/LSB
244ug/LSB

矽睿

1000
1111
Others

The interrupt for the new data serves for the synchronous data reading for the host. It is generated after storing a new
value of z-axis acceleration data into data register. This interrupt will be cleared automatically when the next data
conversion cycle starts, and the interrupt will be effective about 64*MCLK, and automatically cleared.
The interrupt mode for the new data is fixed to be non-latched.

7.3  ANY_MOT_INT

Any motion Any motion detection uses slope between two successive data to detect the changes in motion. It generates
interrupt when a preset threshold ANY_MOT_TH (0x2E) is exceeded.

The time difference between two successive data depends on the output data rate (ODR).

Slope(t1) = (𝑎𝑐𝑐(𝑡1) − 𝑎𝑐𝑐(𝑡0)) ∗ 𝑂𝐷𝑅

The any motion detection criteria are fulfilled and interrupt is generated if any of enabled channels exceeds ANY_MOT_TH
for ANY_MOT_DUR (0x2C<1:0>) consecutive times.

As long as all the enabled channels data fall or stay below ANY_MOT_TH for ANY_MOT_DUR consecutive times, the
interrupt will be reset unless the interrupt signal is latched.

The any motion detection engine will send out the signals of axis which triggered the interrupt (ANY_MOT_FIRST_X
(0x09<0>), ANY_MOT_FIRST_Y (0x09<1>), ANY_MOT_FIRST_Z (0x09<2>)) and the sign of the motion (ANY_MOT_SIGN
(0x09<3>))

The information contained herein is the exclusive property of QST, and shall not be distributed,
reproduced, or disclosed in whole or in part without prior written permission of QST.

  16  /  40

Document #:  13-52-21  Title:  QMA6100 Datasheet

Rev: A

矽睿

There is an option for using any motion detector to detect high-g.

If the 0x2F<6> (any_mot_in_sel) is logic-1, the input of any-motion detector would be acceleration, and the threshold range
would cover full scale range.

The information contained herein is the exclusive property of QST, and shall not be distributed,
reproduced, or disclosed in whole or in part without prior written permission of QST.

  17  /  40

accelerationslopetimetime+threshould-threshouldcountertimedurationdurationdurationtimeinterruptAny MotionDetectorany_mot_in_selAccelSlope of Accel0101ANY_MOT_TH<7:0>*16 LSBANY_MOT_TH<7:0>*32 LSBThreshold of Any Motionany_mot_in_sel (0x2F<6>) : 0 for any motion detection                                                1 for high-g detection

Document #:  13-52-21  Title:  QMA6100 Datasheet

Rev: A

矽睿

7.4  SIG_MOT_INT

A significant motion is a motion due to a change in user location.
The algorithm is as following:
1)  Look for movement, same setting as any motion detection
2)
If movement detected, sleep for T_Skip (0x2F<3:2>)
3)  Look for movement

a)
b)

If no movement detected within T_Proof (0x2F<5:4>), go back to 1
If movement detected, report a significant movement, and generate the interrupt

The significant motion detection and any motion detection are exclusive, user can select either one through SIG_MOT_SEL
(0x2F<0>).
If significant motion is detected, the engine will set SIG_MOT_INT (0x0A<0>).

7.5  NO_MOT_INT

No-motion interrupt is generated if the slope (absolute value of acceleration difference) on all selected axes is smaller than
the programmable threshold for a programmable time. Figure shows the timing for the no-motion interrupt. Register (0x2C)
NO_MOT_DUR defines the delay times before the no-motion interrupt is generated. Table lists the delay times adjustable
with register (0x2C) NO_MOT_DUR.
The no-motion interrupt is enabled per axis by writing logic 1 to bits (0x18) NO_MOTION_EN_X, (0x18) NO_MOTION_EN_Y,
and (0x18) NO_MOTION_EN_Z, respectively. The no-motion threshold is set through the (0x2D) NO_MOT_TH register. The
meaning of an LSB of (0x2D) NO_MOT_TH depends on the selected g-range: it corresponds to 3.91mg in 2g-range (7.81mg
in 4g-range, 15.6mg in 8g-range, 31.25mg in 16g-range, 62.5mg in 32g-range). Therefore the maximum value is 996mg in
2g-range (2g in 4g-range, 4g in 8g-range, 8g in 16g-range, and 16g in 32g-range). The time difference between the
successive acceleration samples depends on the selected ODR and equates to 1/ODR.

The information contained herein is the exclusive property of QST, and shall not be distributed,
reproduced, or disclosed in whole or in part without prior written permission of QST.

  18  /  40

any-motion resulttimetimeinterruptT_skipT_proof(sleep)(detect)1/ODR

Document #:  13-52-21  Title:  QMA6100 Datasheet

Rev: A

矽睿

7.6  TAP_INT

Tap detection allows the device to detect the events such as clicking or double clicking of a touch-pad. A tap event is detected if

a pre-defined slope of the acceleration. The tap detection includes single tap (S_TAP), double tap (D_TAP), triple tap (T_TAP), and
quadruple tap (Q_TAP). A ‘Single tap’ is a single event within a certain shock time, followed by a certain quiet time. A ‘double tap’
consists of a first such event followed by a second event within a defined time frame, and so on.

Each tap interrupt can be enabled (disabled) by setting ‘1’ (‘0’) to S_TAP_EN(0x16<7>), D_TAP_EN(0x16<5>),

T_TAP_EN(0x16<4>), and Q_TAP_EN(0x16<0>).

The status of each tap interrupt is stored in S_TAP_INT(0x0A<7>), D_TAP_INT(0x0A<5>), T_TAP_INT(0x0A<4>), and

Q_TAP_INT(0x0B<0>).

The shock and quiet threshold for detecting a tap event is set by register (0x2B) TAP_SHOCK_TH and (0x1E) TAP_QUIET_TH.

The meaning of threshold LSB is 31.25mg, the range is 0 ~ 2G.

The tap input selection is defined in (0x2B<7:6>) TAP_IN_SEL, the default input is  √𝑥2 + 𝑦2 + 𝑧2, the tap detector could only

detect 1 axis as shown below:
TAP_IN_SEL<1:0>:
0: X-axis
1: Y-axis
2: Z-axis
3:  √𝑥2 + 𝑦2 + 𝑧2

In figure the timing for tap is visualized:

The information contained herein is the exclusive property of QST, and shall not be distributed,
reproduced, or disclosed in whole or in part without prior written permission of QST.

  19  /  40

accelerationslopetime+threshould-threshouldcountertimedurationdurationtimeinterrupttime`

Document #:  13-52-21  Title:  QMA6100 Datasheet

Rev: A

矽睿

7.7  RAISE_INT

Raise wake algorithm is used to detect the action of raise hand (or hand down). The interrupt is enabled by writing logic 1 to
bits (0X16[1]) RAISE_EN, (0X16[2]) HD_EN. User can adjust the sensitivity through the registers. The register
RAISE_WAKE_SUM_TH(0X22[5:0]) defines the strength of hand action (raise and down). The register
RAISE_DIFF_TH(0X23[1:0],0X22[7:6]) defines the differential values of twice actions, when the hand behavior almost done
the differential value will be smaller and we can use this register to set the threshold. RAISE_WAKE_PERIOD and
RAISE_WAKE_TIMEOUT_TH define the duration of the total hand action.

7.8  FIFO_INT

This device has integrated FIFO memory, capable of storing up to 64 frames, with each frame contains three 14bits words,
for acceleration data of X, Y, and Z axis. All of the 3-axes acceleration is sampled at same time point

The FIFO can be configured as three modes, FIFO mode, STREAM mode, and BYPASS mode.
FIFO mode.
In FIFO mode, the acceleration data of selected axes are stored in the buffer memory. If enabled, a watermark interrupt can
be triggered when the buffer filled up to the defined level. The buffer will continuously be filled until the fill level reaches to
64. When the buffer is full, data collection stops, and the new data will be ignored. Also, FIFO_FULL interrupt will be
triggered when enabled.
STREAM mode
In STREAM mode, the acceleration data of selected axes will be stored into the buffer until the buffer is full. The buffer’s
depth is 64 now. when the buffer is full, data collection continues, and the oldest data is discarded. If enabled, a watermark
interrupt will be triggered when the fill level reached to the defined level. Also, when buffer is full, FIFO_FULL interrupt will
be triggered if enabled. If any old data is discarded, the FIFO_OR (0x0B<7>) will be set to be logic 1.
BYPASS mode
In BYPASS mode, only the current acceleration data of selected axes can be read out from FIFO. The FIFO acts like the
STREAM mode when a depth of 1. Compared to reading directly from data register, this mode has the advantage of
ensuring the package of xyz data are from same time point. The data registers are updated sequentially and have chance for

The information contained herein is the exclusive property of QST, and shall not be distributed,
reproduced, or disclosed in whole or in part without prior written permission of QST.

  20  /  40

Slopetime+Shock Threshould－Shock ThreshouldSingle Tap Interruptshock timeDouble Tap Interrupt12.5msTriple Tap Interruptduration timequiet timeshock timequiet timeshock timequiet timeshock timequiet time12.5ms12.5msQuadruple Tap Interrupt12.5mstimetimetimetimeduration timeduration time+Quiet Threshould－Quiet Threshould

Document #:  13-52-21  Title:  QMA6100 Datasheet

Rev: A

矽睿

xyz data are from different time. Also, if any old data is discarded, the FIFO_OR will be set to be logic 1, similar as that in
STREAM mode.
The FIFO mode can be configured by setting FIFO_MODE (0x3E<7:6>).

FIFO_MODE

00
01
10
11

MODE
BYPASS
FIFO
STREAM
FIFO

User can select the acceleration data of which axes to be stored in FIFO. This configuration can be done by setting FIFO_CH
(0x3E<2:0>)
If all of the 3-axes data are selected, the format of data read from 0x3F is as following

XLSB

XMSB

YLSB

YMSB

ZLSB

ZMSB

These comprise one frame

If only one axis is enabled, the format data read from 0x3F is as following

YLSB

YMSB

These comprise one frame

If the frame is not read completely, the remaining parts of the frame will be discarded.
If the FIFO is read beyond the FIFO fill level, all zeroes will be read out.

FIFO_FRAME_COUNTER (0x0E<7:0>) reflects the current filled level of the buffer. If additional data frames are written into
the buffer when FIFO is full (in STREAM mode or BYPASS mode), then FIFO_OR (0x0B<7>) is set to be logic 1. This FIFO_OR
bit can be considered as flag of discarding old data.
When a write access to one of the FIFO configuration registers (0x3E) or watermark registers (0x31) occurs, the FIFO buffer
will be cleared, the FIFO fill level indication register FIFO_FRAME_COUNTER (0x0E<7:0>) will be cleared, and the FIFO_OR
(0x0B<7>) will be cleared as well.

As mentioned above, FIFO controller contains two interrupts, FIFO_FULL interrupt and watermark interrupt. These two
interrupts are functional in all of the FIFO operating modes.
The watermark interrupt is triggered when the filled level of buffer reached to the level that is defined by register
FIFO_WM_LVL (0x31<7:0>), if the interrupt is enabled by setting INT_FWM_EN (0x17<6>) to logic 1 and INT1_FWM
(0x1A<6>) or INT2_FWM (0x1C<6>) is set.
The FIFO_FULL interrupt is triggered when the buffer has been fully filled. In FIFO mode, the filled level is 64, and in STREAM
mode the filled level is 64, in BYPASS mode the filled level is 1. To enable FIFO_FULL interrupt, INT_FFULL_EN (0x17<5>)
should be set to 1, and INT1_FFULL (0x1A<5>) and INT2_FFULL (0x1C<5>) is set.

The status of watermark interrupt and FIFO full interrupt can be read through INT_STAT (0x0B)
After soft-reset, the watermark interrupt and FIFO full interrupt are disabled.

For the FIFO to recollect the data, user should reconfigure the register FIFO_MODE.

7.9  Interrupt configuration

The device has the above 3 interrupt engines. Each of the interrupts can be enabled and configured independently. If the
trigger condition of the enabled interrupt fulfilled, the corresponding interrupt status bit will be set to logic 1, and the
mapped interrupt pin will be activated. The device has two interrupt PINs, INT1 and INT2. Each of the interrupts can be
mapped to either PIN or both PINs.

The interrupt status registers INT_ST(0x09~0x0d) will update when a new data word is written into the data registers. If an
interrupt is disabled, the related active interrupt status bit is disabled immediately.

The information contained herein is the exclusive property of QST, and shall not be distributed,
reproduced, or disclosed in whole or in part without prior written permission of QST.

  21  /  40

Document #:  13-52-21  Title:  QMA6100 Datasheet

Rev: A

矽睿

When interrupt condition is fulfilled, related bit of interrupt will be set, until the associated interrupt condition is no more
valid. Read operation to related register will also clear the register.

Device supports 2 interrupt modes, non-latched, and latched mode. The interrupt modes are set through LATCH_INT
(0x21<0>).

In non-latched mode, the mapped interrupt pin will be set and/or cleared same as associated interrupt register bit. Also, the
mapped interrupt pin can be cleared with read operation to any of the INT_ST(0x09~0x0d).

Exception to this is the new data interrupt and step interrupt, which are automatically reset after a fixed time (T_Pulse =
64/MCLK), no matter LATCH_INT (0x21<0>) is set to 0 or 1.

In latched mode, the clearings of mapped pins are determined by INT_RD_CLR (0x21<7>).
If the condition for trigging the interrupt still holds, the interrupt status will be set again with the next change of the data
registers.

Mapping the interrupt pins can be set by INT_MAP (0x19~0x1B).

The electrical interrupt pins can be set INT_PIN_CONF (0x20<3:0>). The active logic level can be set to 1 or 0, and the
interrupt pin can be set to open-drain or push-pull.

The information contained herein is the exclusive property of QST, and shall not be distributed,
reproduced, or disclosed in whole or in part without prior written permission of QST.

  22  /  40

Document #:  13-52-21  Title:  QMA6100 Datasheet

Rev: A

矽睿

8      DIGITAL INTERFACE

8.1

I2C Timings

Table 9 and Figure 11 describe the I2C communication protocol times

Table 9.    I2C Timings

Parameter
SCL Clock
SCL Low Period
SCL High Period

SDA Setup Time
SDA Hold Time
Start Hold Time
Start Setup Time
Stop Setup Time
New Transmission
Time
Rise Time
Fall Time

Symbol
fscl
tlow
thigh

tsudat
thddat
thdsta
tsusta
tsusto
tbuf

tr
tf

Condition

Typ.

Max.
400

0.9

Min.
0
1
1

0.1
0
0.6
0.6
0.6
1.3

Unit
kHz
μs

μs

μs
μs
μs
μs
μs

μs

μs
μs

Figure 11.    I2C Timing Diagram

8.2

I2C R/W Operation

8.2.1

Abbreviation

Table 10.    Abbreviation

The information contained herein is the exclusive property of QST, and shall not be distributed,
reproduced, or disclosed in whole or in part without prior written permission of QST.

  23  /  40

Document #:  13-52-21  Title:  QMA6100 Datasheet

Rev: A

矽睿

SACK
MACK
NACK
RW

Acknowledged by slave
Acknowledged by master
Not acknowledged by master
Read/Write

8.2.2

Start/Stop/Ack

START: Data transmission begins with a high to transition on SDA while SCL is held high. Once I2C transmission starts, the
bus is considered busy.
STOP:  STOP condition is a low to high transition on SDA line while SCL is held high.
ACK: Each byte of data transferred must be acknowledged. The transmitter must release the SDA line during the
acknowledge pulse while the receiver must then pull the SDA line low so that it remains stable low during the high period of
the acknowledge clock cycle.
NACK: If the receiver doesn’t pull down the SDA line during the high period of the acknowledge clock cycle, it’s recognized
as NACK by the transmitter.

8.2.3

I2C Write

I2C write sequence begins with start condition generated by master followed by 7 bits slave address and a write bit
(R/W=0). The slave sends an acknowledge bit (ACK=0) and releases the bus. The master sends the one-byte register
address. The slave again acknowledges the transmission and waits for 8 bits data which shall be written to the specified
register address. After the slave acknowledges the data byte, the master generates a stop signal and terminates the writing
protocol.

Table 11.      I2C Write

Slave Address

S
T
A
R
T

0  0  1  0  0  1  0  0

R
W  S
A
C
K

Register Address
(0x11)
0  0  0  1  0  0  0  1

S
A
C
K

Data
(0x80)
1  0  0  0  0  0  0  0

S
A
C
K

S
T
O
P

8.2.4

I2C Read

I2C write sequence consists of a one-byte I2C write phase followed by the I2C read phase. A start condition must be
generated between two phases. The I2C write phase addresses the slave and sends the register address to be read. After
slave acknowledges the transmission, the master generates again a start condition and sends the slave address together
with a read bit (R/W=1). Then master releases the bus and waits for the data bytes to be read out from slave. After each
data byte the master has to generate an acknowledge bit (ACK = 0) to enable further data transfer. A NACK from the master
stops the data being transferred from the slave. The slave releases the bus so that the master can generate a STOP
condition and terminate the transmission.
The register address is automatically incremented and more than one byte can be sequentially read out. Once a new data
read transmission starts, the start address will be set to the register address specified in the current I2C write command.

Table 12.    I2C Read

Slave Address

S
T
A
R
T

0  0  1  0  0  1  0  0

R
W  S
A
C
K

Register Address
(0x00)
0  0  0  0  0  0  0  0

S
A
C
K

T

A
R

S
T

Slave Address

R
C
K
W

S
A

Data
(0x00)

K

MA
C

Data
(0x01)

The information contained herein is the exclusive property of QST, and shall not be distributed,
reproduced, or disclosed in whole or in part without prior written permission of QST.

  24  /  40

Document #:  13-52-21  Title:  QMA6100 Datasheet

Rev: A

矽睿

0  0  1  0  0  1  0  1

0  0  0  0  0  0  1  0

0  0  0  0  0  0  0  0

Data
(0x02)
0  0  0  0  0  0  1  0

M
A
C
K

M
A
C
K

……………………………….

……………………………….

Data
(0x07)
0  0  0  0  0  0  0  0

N
A
C
K

S
T
O
P

M
A
C
K

8.3

Serial Peripheral Interface(SPI)

The timing specification of SPI is given in the following table.

Table 13: SPI timing

Parameter
Clock Frequency

SCK Low Pulse
SCK High Pulse

SDI Setup Time
SDI Hold Time
SDO Output Delay

Symbol
fSPI

Condition
Max. load on SDI or
SDO=25pF

Min.
0

Max.
10

Unit
MHz

tSCKL
tSCKH

tSDI_setup
tSDI_hold
tSDO_OD

Load =25pF
Load =250pF,
Vddio =2.4V

20
20

20
20

20
40

ns

ns

ns
ns
ns
ns

ns
ns

30
40

SENB Setup Time
SENB Hold Time

tSENB_setup
tSENB_hold

The following figure shows the definition of SPI timing given in table 13:

Figure. 12 SPI timing diagram

The SPI interface of QMA6100 is compatible with mode ‘11’. Two configurations of SPI interface are supported by QMA6100: 4-wire
and 3-wire. The same protocol is used by both configurations. The device operates in 4-wire configuration by default. The
configuration can be switched to 3-wire configuration by setting EN_SPI3W(0x20[5])=1. Pin SDI is used as the common data pin in 3-
wire configuration.

For single byte read or write operation, 16-bit protocols are used. QMA6100 also supports multiple-byte read or write operations.

The information contained herein is the exclusive property of QST, and shall not be distributed,
reproduced, or disclosed in whole or in part without prior written permission of QST.

  25  /  40

SENBSCKSDISDOtSENB_setuptSENB_holdtSDI_setuptSDI_holdtSDO_ODtSCKLtSCKH

Document #:  13-52-21  Title:  QMA6100 Datasheet

Rev: A

矽睿

In 4-wire configuration, SENB(low active), SCK(serial clock), SDI(serial data input) and SDO(serial data output) pins are used. The
communication starts when SENB is pulled low by SPI master and stops when SENB is pulled high. SCK is also controlled by SPI
master. SDI and SDO are driven at the falling edge of SCK and should be captured at the rising edge of SCK.

The basic write operation waveform for 4-wire configuration is depicted below in figure 13. During the entire write cycle SDO
remains in high impedance state.

The basic read operation waveform for 4-wire configuration is depicted in figure 14 below.

Figure 13: 4-wire basic SPI Write sequence

The data bits are defined as follows:

Figure 14: 4-wire basic SPI Read sequence

Bit0: Read/Write bit. When 0, the data DI is written to the chip. When 1, the data DO is read from the chip.

Bit1-7: Address A(6:0).

Bit8-15: when in write mode, these are the data DI, which will be written to the address. When in read mode, these are the DO,
which are read from the address.

The information contained herein is the exclusive property of QST, and shall not be distributed,
reproduced, or disclosed in whole or in part without prior written permission of QST.

  26  /  40

R/WA6A5A4A3A2A1A0DI7DI6DI5DI4DI3DI2DI1DI0SENBSCKSDISDO4-Wire SPI Write Timing Diagram (Mode 3)Bit 0 : R/W bit,  R/W=0 : write mode;  R/W=1 : read mode.Bit 1-7: 7-bit address of registers.Bit 8-15: Data DI7~DI0 (write mode). It is the data that will be written into the slave. (MSB first)Bit 8-15: Data DO7~DO0 (read mode). It is the data that will be read from the device. (MSB first)R/WA6A5A4A3A2A1A0DI7DI6DI5DI4DI3DI2DI1DI0DO7DO6DO5DO4DO3DO2DO1DO0SENBSCKSDISDO4-Wire SPI Read Timing Diagram (Mode 3)Bit 0 : R/W bit,  R/W=0 : write mode;  R/W=1 : read mode.Bit 1-7: 7-bit address of registers.Bit 8-15: Data DI7~DI0 (write mode). It is the data that will be written into the slave. (MSB first)Bit 8-15: Data DO7~DO0 (read mode). It is the data that will be read from the device. (MSB first)

Document #:  13-52-21  Title:  QMA6100 Datasheet

Rev: A

矽睿

Multiple byte read/write operations are possible by keeping SENB low and continuing the data transfer. Only the first register
address has to be provided. Addresses are automatically incremented after each read/write access as long as SENB stays low.

The principle of multiple read/write is shown below.

Figure 15: SPI multiple byte Read/Write

In 3-wire configuration, SENB(low active), SCK(serial clock) and SDI(serial data input) pins are used. The communication starts when
SENB is pulled low by SPI master and stops when SENB is pulled high. SCK is also controlled by SPI master. SDI is driven at the falling
edge of SCK when used as input of the device and should be captured at the rising edge of SCK when used as the output of the
device.

The information contained herein is the exclusive property of QST, and shall not be distributed,
reproduced, or disclosed in whole or in part without prior written permission of QST.

  27  /  40

SCKBurst Read Timing DiagramDataR from RegindexR, Index AddressDataR from Regindex+1DataR from Regindex+2SENBSDISDOSCKBurst Write Timing DiagramDataw to RegindexW, Index AddressDataw to Regindex+1Dataw to Regindex+2SENBSDISDOR/WA6A5A4A3A2A1A0D7D6D5D4D3D2D1D0SENBSCKSDI3-Wire SPI Read/Write Timing Diagram  (Mode 3)Bit 0 : R/W bit,  R/W=0 : write mode;  R/W=1 : read mode.Bit 1-7: 7-bit address of registers.Bit 8-15: R/W=0 : D7 ~ D0 are written into slave;    R/W=1 : D7~D0 are read from slave. (MSB first).

Document #:  13-52-21  Title:  QMA6100 Datasheet

Rev: A

矽睿

Figure 16: 3-wire basic SPI Read/Write sequence

9    REGISTERS

9.1

Register Map

The table below provides a list of the 8-bit registers embedded in the device and their respective function and addresses

Table 14. Register Map

The information contained herein is the exclusive property of QST, and shall not be distributed,
reproduced, or disclosed in whole or in part without prior written permission of QST.

  28  /  40

Document #:  13-52-21  Title:  QMA6100 Datasheet

Rev: A

矽睿

The information contained herein is the exclusive property of QST, and shall not be distributed,
reproduced, or disclosed in whole or in part without prior written permission of QST.

  29  /  40

Add.Namebit7bit6bit5bit4bit3bit2bit1bit0R/WDEF0x3FR000x3EFIFO_EN_ZFIFO_EN_YFIFO_EN_XRW070x3DRWNVM0x3CRWNVM0x3BRWNVM0x3ARWNVM0x39RWNVM0x38RWNVM0x37RWNVM0x36S_RESETRW000x35RW660x34RW9D0x33InternalRW050x32STSELFTEST_BITSELFTEST_SIGNRW000x31FIFO_WMRW000x30RST_MOTMO_BP_LPFSTEP_BP_LPFTAP_RST_NNO_MOT_RST_NSIG_MOT_RST_NANY_MOT_RST_NRW3F0x2FRFF_BP_LPFANY_MOT_IN_SELSIG_MOT_SELRW000x2ERW000x2DRW000x2CRW000x2BRWCD0x2ATAP_QUIETTAP_SHOCKT_TAP_DELAYTAP_EARINRW050x29RW000x28RW000x27RW000x26RAISE_MODERW020x25RW810x24RW000x23RW7C0x22RWD80x21INT_CFGINT_RD_CLRSHADOW_DISDIS_I2CLATCH_INT_STEPLATCH_INTRW000x20INT_PIN_CFGDIS_PU_SENBDIS_IE_AD0EN_SPI3WSTEP_COUNT_PEAK<2>INT2_ODINT2_LVLINT1_ODINT1_LVLRW050x1FRWA90x1ERW080x1DEN_RESET_DCRW000x1CINT2_NO_MOTINT2_FWMINT2_FFULLINT2_DATAINT2_Q_TAPINT2_ANY_MOTRW000x1BINT2_S_TAPINT2_SIG_STEPINT2_D_TAPINT2_T_TAPINT2_STEPINT2_HDINT2_RAISEINT2_SIG_MOTRW000x1AINT1_NO_MOTINT1_FWMINT1_FFULLINT1_DATAINT1_Q_TAPINT1_ANY_MOTRW000x19INT1_S_TAPINT1_SIG_STEPINT1_D_TAPINT1_T_TAPINT1_STEPINT1_HDINT1_RAISEINT1_SIG_MOTRW000x18NO_MOT_EN_ZNO_MOT_EN_YNO_MOT_EN_XANY_MOT_EN_ZANY_MOT_EN_YANY_MOT_EN_XRW000x17INT_FWM_ENINT_FFULL_ENINT_DATA_ENRW000x16S_TAP_ENSIG_STEP_IEND_TAP_ENT_TAP_ENSTEP_IENHD_ENRAISE_ENQ_TAP_ENRW000x15RW160x14RW190x13STEP_CLRRW7F0x12STEP_ENRW140x11PMMODE_BITRW000x10BWRW000x0FFSREN_16BRW000x0EFIFO_STR000x0DR000x0CTAP_SIGNR000x0BFIFO_ORFIFO_WM_INTFIFO_FULL_INTDATA_INTEARIN_FLAGQ_TAP_INTR000x0AS_TAP_INTSIG_STEPD_TAP_INTT_TAP_INTSTEP_INTHD_INTRAISE_INTSIG_MOT_INTR000x09NO_MOTSTEP_FLAGANY_MOT_SIGNANY_MOT_FIRST_ZANY_MOT_FIRST_YANY_MOT_FIRST_XR000x08R000x07R000x06R000x050NEWDATA_ZR000x04R000x030NEWDATA_YR000x02R000x010NEWDATA_XR000x00CHIP IDRANAFIFO_CFGFIFO_DATA<7:0>FIFO_MODE<1:0>RAISE_XYZ_SW<2:0>IMAGEGAIN_Z<7:0>GAIN_Y<7:0>GAIN_X<7:0>OFFSET_Z<7:0>Z_TH[3:0]X_TH[3:0]YZ_TH_SEL[2:0]Y_TH[4:0]OFFSET_Y<7:0>OFFSET_X<7:0>OFFSET_X<10:8>GAIN_Z<9:8>OFFSET_Y<10:8>SOFTRESET: 0xB6 / NVM_UNLOCK: 0xB3TAPTAP_IN_SEL<1:0>TAP_SHOCK_TH<5:0>TAP_DUR<2:0>OS_CUSTOS_CUST_Z<7:0>OS_CUST_Y<7:0>OS_CUST_X<7:0>STEP_BP_AXIS<1:0>FIFO_WTMK_LVL<7:0>MOT_CFGSIG_MOT_TPROOF<1:0>SIG_MOT_TSKIP<1:0>ANY_MOT_TH<7:0>NO_MOT_TH<7:0>NO_MOT_DUR<5:0>ANY_MOT_DUR<1:0>STEP_CFGSTEP_START_CNT<2:0>STEP_COUNT_PEAK<1:0>STEP_COUNT_P2P<2:0>NLPF_STEP<1:0>TAP_QUIET_TH[5:0]STEP_INTERVAL<6:0>naRAISE_WAKE_PERIOD[10:8]RAISE_WAKE_TIMEOUT_TH[11:8]RAISE_WAKE_PERIOD[7:0]RAISE_WAKE_TIMEOUT_TH[7:0]HD_Z_TH[2:0]HD_X_TH[2:0]RAISE_WAKE_DIFF_TH[3:2]RAISE_WAKE_DIFF_TH[1:0]RAISE_WAKE_SUM_TH[5:0]T_RSTB_SINC_SEL<1:0>MCLK_SEL<3:0>NLPF<1:0>BW<4:0>RANGE<3:0>FIFO_FRAME_COUNTER<7:0>INT_MAPINT_ENnaSTEP_TIME_UP<7:0>STEP_TIME_LOW<7:0>STEP_PRECISION<6:0>STEP_SAMPLE_CNT<6:0>ACC_X<13:6>ACC_X<5:0>CHIP ID to indicate the product versionINT_STSTEP_CNT<23:16>naSTEP_CNT<15:8>STEP_CNT<7:0>DATAACC_Z<13:6>ACC_Z<5:0>ACC_Y<13:6>ACC_Y<5:0>

Document #:  13-52-21  Title:  QMA6100 Datasheet

Rev: A

矽睿

9.2

Register Definition

Register 0x00 (CHIP ID)

Bit6

Bit7
CHIP_ID<7:0>
This register is used to identify the device

Bit5

Bit4

Bit3

Bit2

Bit1

Bit0

Register 0x01 ~ 0x02 (DXL, DXM)

Bit7
DX<5:0>

DX<13:6>

DX:
NEWDATA_X:

Bit6

Bit5

Bit4

Bit3

Bit2

Bit1

14bits acceleration data of x-channel. This data is in two’s complement.
1, acceleration data of x-channel has been updated since last reading
0, acceleration data of x-channel has not been updated since last reading

Register 0x03 ~ 0x04 (DYL, DYM)

Bit7
DY<5:0>

DY<13:6>

DY:
NEWDATA_Y:

Bit6

Bit5

Bit4

Bit3

Bit2

Bit1

14bits acceleration data of y-channel. This data is in two’s complement.
1, acceleration data of y-channel has been updated since last reading
0, acceleration data of y-channel has not been updated since last reading

Register 0x05 ~ 0x06 (DZL, DZM)

Bit7
DZ<5:0>

DZ<13:6>

DZ:
NEWDATA_Z:

Bit6

Bit5

Bit4

Bit3

Bit2

Bit1

14bits acceleration data of z-channel. This data is in two’s complement.
1, acceleration data of z-channel has been updated since last reading
0, acceleration data of z-channel has not been updated since last reading

Bit0
NEWDATA
_X

Bit0
NEWDATA
_Y

Bit0
NEWDATA
_Z

Register 0x07 ~ 0x08 (STEP_CNT)

Bit6

Bit7
STEP_CNT<7:0>
STEP_CNT<15:8>
STEP_CNT<15:0>:

Bit5

Bit4

Bit3

Bit2

Bit1

Bit0

16 bits of step counter, out of total 24bits data. The MSB data are in 0x0e

Register 0x09 (INT_ST0)

Bit7
NO_MOT

Bit6
STEP_FLAG

Bit5

Bit4

NO_MOT:

STEP_FLAG:

1, no_motion interrupt active
0, no_motion interrupt inactive
1, STEP detected
0, STEP not detected

Bit3
ANY_MOT
_SIGN

Bit2
ANY_MOT
_FIRST_Z

Bit1
ANY_MOT
_FIRST_Y

Bit0
ANY_MOT
_FIRST_X

ANY_MOT_FIRST_Z:

ANY_MOT_SIGN:              1, sign of any_motion triggering signal is negative
0, sign of any_motion triggering signal is positive
1, any_motion interrupt is triggered by Z axis
0, any_motion interrupt is not triggered by Z axis
1, any_motion interrupt is triggered by Y axis
0, any_motion interrupt is not triggered by Y axis
1, any_motion interrupt is triggered by X axis
0, any_motion interrupt is not triggered by X axis

ANY_MOT_FIRST_X:

ANY_MOT_FIRST_Y:

R/W
RW

R/W
R

R

R/W
R

R

R/W
R

R

R/W
R
R

R/W
R

Default
0xFA

Default
0x00

0x00

Default
0x00

0x00

Default
0x00

0x00

Default
0x00
0x00

Default
0x00

Register 0x0a (INT_ST1)

Bit7
S_TAP_INT

Bit6
SIG_STEP

Bit5
D_TAP_INT

Bit4
T_TAP_INT

Bit3
STEP_INT

Bit2
HD_INT

Bit1
RAISE_INT

Bit0
SIG_MOT_I
NT

R/W
R

Default
0x00

S_TAP_INT:

1, single tap is active
0, single tap is inactive

The information contained herein is the exclusive property of QST, and shall not be distributed,
reproduced, or disclosed in whole or in part without prior written permission of QST.

  30  /  40

Document #:  13-52-21  Title:  QMA6100 Datasheet

Rev: A

矽睿

SIG_STEP:

D_TAP_INT:

STEP_INT:

T_TAP_INT:

1, significant step is active
0, significant step is inactive
1, double tap is active
0, double tap is inactive
1, step valid interrupt is active
0, step quit interrupt is inactive
1, triple tap is active
0, triple tap is inactive

HD_INT:

                    1, hand down interrupt is active

RAISE_INT:

0, hand down interrupt is inactive
1, raise hand interrupt is active
                              0, raise hand interrupt is inactive

SIG_MOT_INT:

        1, significant interrupt is active

0, significant interrupt is inactive

Register 0x0b (INT_ST2)

Bit7
FIFO_OR

FIFO_OR:

FIFO_WM_INT:

FIFO_FULL_INT:

DATA_INT:

EARIN_FLAG:

Q_TAP_INT:

Bit6
FIFO_WM_
INT

Bit5
FIFO_FULL
_INT

Bit4
DATA_INT

Bit3

Bit2

Bit1
EARIN_FLA
G

Bit0
Q_TAP_INT

R/W
R

Default
0x00

1, FIFO Over-Run occurred
0, FIFO Over-Run not occurred
1, FIFO watermark interrupt is active
0, FIFO watermark interrupt is inactive
1, FIFO full interrupt is active
0, FIFO full interrupt is inactive
1, data ready interrupt active
0, data ready interrupt inactive
1, ear-in interrupt is active
0, ear-in interrupt is inactive
1, quad tap is active
0, quad tap is inactive

Register 0x0c (INT_ST3)
Bit6

Bit7
TAP_SIGN
TAP_SIGN:

Register 0x0d (INT_ST4)
Bit6

Bit7
STEP_CNT<23:16>
STEP_CNT<23:16>:

Register 0x0e (FIFO_ST)

Bit5

Bit4

Bit3

Bit2

Bit1

Bit0

1, tap sign is along with positive direction
0, tap sign is along with negative direction

Bit5

Bit4

Bit3

Bit2

Bit1

Bit0

8bit MSB data of step counter, out of total 24bits data. The LSB data are in 0x07 and 0x08

R/W
R

R/W
R

Default
0x00

Default
0x00

Bit7
FIFO_FRAME_COUNTER<7:0>

Bit6

Bit5

Bit4

Bit3

Bit2

Bit1

Bit0

R/W
R

Default
0x00

FIFO_FRAME_COUNTER<7:0>:  Fill level of FIFO buffer. An empty FIFO corresponds to 0x00. The frame counter can be cleared by reading out all of the frames, or by

writing register 0x3e (FIFO_CFG1) or 0x21.

Register 0x0f (FSR)

Bit7

Bit6

RANGE<3:0>:

Register 0x10 (BW)

Bit5

Bit4

Bit2

Bit1

Bit0

Bit3
RANGE<3:0>

set the full scale of the accelerometer. Setting as following

R/W
RW

Default
0x00

RANGE<3:0>
0001
0010
0100
1000
1111
Others

Acceleration range
2g
4g
8g
16g
32g
2g

Resolution
244ug/LSB
488g/LSB
977ug/LSB
1.95mg/LSB
3.91mg/LSB
244ug/LSB

Bit7

Bit6
NLPF<1:0>

Bit5

Bit4
BW<4:0>

Bit3

Bit2

Bit1

Bit0

R/W
RW

Default
0xE0

The information contained herein is the exclusive property of QST, and shall not be distributed,
reproduced, or disclosed in whole or in part without prior written permission of QST.

  31  /  40

Document #:  13-52-21  Title:  QMA6100 Datasheet

Rev: A

矽睿

NLPF<1:0>:

BW<4:0>:

00, no LPF
01, average = 2
10, average = 4
11, average = 16
bandwidth setting, as following

BW<4:0>
xx000
xx001
xx010
xx011
xx100
xx101
xx110
xx111
Others

ODR
MCLK/512
MCLK/256
MCLK/128
MCLK/64
MCLK/32
MCLK/1024
MCLK/2048
MCLK/4096
MCLK/512

ODR (MCLK = 500kHz)
977 Hz
1953 Hz
3906 Hz
7813 Hz
15625 Hz
488 Hz
244 Hz
122 Hz
977 Hz

Register 0x11 (PM)

Bit5
T_RSTB_SINC_SEL<1:0>

Bit4

Bit3
MCLK_SEL<3:0>

Bit2

Bit1

Bit0

R/W
RW

Default
0x00

Bit6

Bit7
MODE_BIT
MODE_BIT:

                    1, set device into active mode

          0, set device into standby mode

T_RSTB_SINC_SEL<1:0>:   Reset clock setting. The preset time is reserved for CIC filter in digital

                              11, T_RSTB_SINC=8*MCLK
                              10, T_RSTB_SINC=6*MCLK
                              01, T_RSTB_SINC=4*MCLK
                              00, T_RSTB_SINC=3*MCLK

MCLK_SEL<3:0>:
MCLK_SEL<3:0>
0000
0001
0010
0011
0100
0101
0110
0111
1xxx

set the master clock to digital

Freq of MCLK
500k
333k
200k
100k
50k
20k
10k
5k
Reserved

Register 0x12 (STEP_CONF0)

Bit4

Bit3

Bit2

Bit1

Bit0

R/W
RW

Default
0x14

enable step counter, this bit should be set 1 when using step counter
sample count setting to renew dynamic threshold. The actual value is STEP_SAMPLE_CNT<6:0>*8, default is 0xC, 96 sample count

Bit6
STEP_SAMPLE_CNT<6:0>

Bit5

Bit7
STEP_EN
STEP_EN:
STEP_SAMPLE_CNT:

Register 0x13 (STEP_CONF1)

Bit6
STEP_PRECISION<6:0>

Bit7
STEP_CLR
STEP_CLR:
STEP_PRECISION<6:0>:  algorithm setting

Bit5

Bit4

Bit3

Bit2

Bit1

Bit0

clear step count in register 0x0D ,0x08 and 0x07

Register 0x14 (STEP_CONF2)

Bit6

Bit7
STEP_TIME_LOW<7:0>
STEP_TIME_LOW<7:0>:  algorithm setting

Bit5

Bit4

Bit3

Bit2

Bit1

Bit0

Register 0x15 (STEP_CONF3)

Bit5

Bit4

Bit3

Bit2

Bit1

Bit0

algorithm setting

Bit6

Bit7
STEP_TIME_UP<7:0>
STEP_TIME_UP<7:0>:

Register 0x16 (INT_EN0)

R/W
RW

Default
0x7F

R/W
RW

R/W
RW

Default
0x19

Default
0x00

The information contained herein is the exclusive property of QST, and shall not be distributed,
reproduced, or disclosed in whole or in part without prior written permission of QST.

  32  /  40

Document #:  13-52-21  Title:  QMA6100 Datasheet

Rev: A

矽睿

Bit7
S_TAP_EN

Bit6
SIG_STEP_I
EN

Bit5
D_TAP_EN

Bit4
T_TAP_EN

Bit3
STEP_IEN

Bit2
HD_EN

Bit1
RAISE_EN

Bit0
Q_TAP_EN

R/W
RW

Default
0x00

S_TAP_EN:

T_TAP_EN:

D_TAP_EN:

SIG_STEP_IEN:

1, enable single tap
0, disable single tap
1, enable significant step interrupt
0, disable significant step interrupt
1, enable double tap
0, disable double tap
1, enable triple tap
0, disable triple tap
1, enable step valid interrupt
0, disable step valid interrupt
1, enable hand-down interrupt
0, disable hand-down interrupt
RAISE_EN:                          1, enable raise-hand interrupt
                                          0, disable raise-hand interrupt
Q_TAP_EN:

STEP_IEN:

HD_EN:

1, enable quad tap
0, disable quad tap

Register 0x17 (INT_EN1)

Bit7

Bit6
INT_FWM_
EN

Bit5
INT_FFULL
_INT

Bit4
INT_DATA
_EN

Bit3

Bit2

Bit1

Bit0

R/W
RW

Default
0x00

INT_FWM_EN:

INT_FFULL_EN:

INT_DATA_EN:

1, enable FIFO watermark interrupt
0, disable FIFO watermark interrupt
1, enable FIFO full interrupt
0, disable FIFO full interrupt

          1, enable data ready interrupt
          0, disable data ready interrupt

Register 0x18 (INT_EN2)

Bit7
NO_MOT_
EN_Z

Bit6
NO_MOT_
EN_Y

Bit5
NO_MOT_
EN_X

Bit4

Bit3

Bit2
ANY_MOT
_EN_Z

Bit1
ANY_MOT
_EN_Y

Bit0
ANY_MOT
_EN_X

R/W
RW

Default
0x00

NO_MOT_EN_Z:

NO_MOT_EN_Y:

NO_MOT_EN_X:

ANY_MOT_EN_Z:

ANY_MOT_EN_Y:

ANY_MOT_EN_X:

          1, enable no_motion interrupt on Z axis
          0, disable no_motion interrupt on Z axis
          1, enable no_motion interrupt on Y axis
          0, disable no_motion interrupt on Y axis
          1, enable no_motion interrupt on X axis
          0, disable no_motion interrupt on X axis
          1, enable any_motion interrupt on Z axis
          0, disable any_motion interrupt on Z axis
          1, enable any_motion interrupt on Y axis
          0, disable any_motion interrupt on Y axis
1, enable any_motion interrupt on X axis
          0, disable any_motion interrupt on X axis

Register 0x19 (INT_MAP0)

Bit7
INT1_S_TA
P

INT1_S_TAP:

INT1_SIG_STEP:

INT1_D_TAP:

INT1_T_TAP:

INT1_STEP:

INT1_HD:

Bit6
INT1_SIG_
STEP

Bit5
INT1_D_TA
P

Bit4
INT1_T_TA
P

Bit3
INT1_STEP

Bit2
INT1_HD

Bit1
INT1_RAIS
E

Bit0
INT1_SIG_
MOT

R/W
RW

Default
0x00

1, map single tap interrupt to INT1 pin
0, not map single tap interrupt to INT1 pin
1, map significant step interrupt to INT1 pin
0, not map significant step interrupt to INT1 pin
1, map double tap interrupt to INT1 pin
0, not map double tap interrupt to INT1 pin
1, map triple tap interrupt to INT1 pin
0, not map triple tap interrupt to INT1 pin
1, map step valid interrupt to INT1 pin
0, not map step valid interrupt to INT1 pin
1, map hand down interrupt to INT1 pin

The information contained herein is the exclusive property of QST, and shall not be distributed,
reproduced, or disclosed in whole or in part without prior written permission of QST.

  33  /  40

Document #:  13-52-21  Title:  QMA6100 Datasheet

Rev: A

矽睿

INT1_RAISE:

INT1_SIG_MOT:

0, not map hand down interrupt to INT1 pin
1, map raise hand interrupt to INT1 pin
0, not map raise hand interrupt to INT1 pin
1, map significant interrupt to INT1 pin
0, not map significant interrupt to INT1 pin

Register 0x1a (INT_MAP1)

Bit7
INT1_NO_
MOT

Bit6
INT1_FWM

Bit5
INT1_FFUL
L

Bit4
INT1_DAT
A

Bit3

Bit2

Bit1
INT1_Q_TA
P

Bit0
INT1_ANY_
MOT

R/W
RW

Default
0x00

INT1_NO_MOT:

          1, map no_motion interrupt to INT1 pin
          0, not map no_motion interrupt to INT1 pin

INT1_FWM:

INT1_FFULL:

INT1_DATA:

INT1_Q_TAP:

INT1_ANY_MOT:

1, map FIFO watermark interrupt to INT1 pin
0, not map FIFO watermark interrupt to INT1 pin
1, map FIFO full interrupt to INT1 pin
0, not map FIFO full interrupt to INT1 pin

          1, map data ready interrupt to INT1 pin
          0, not map data ready interrupt to INT1 pin
1, map quad tap interrupt to INT1 pin
0, not map quad tap interrupt to INT1 pin

          1, map any motion interrupt to INT1 pin
          0, not map any motion interrupt to INT1 pin

Register 0x1b (INT_MAP2)

Bit7
INT2_S_TAP

INT2_S_TAP:

INT2_SIG_STEP:

INT2_D_TAP:

INT2_T_TAP:

INT2_STEP:

INT2_HD:

INT2_RAISE:

INT2_SIG_MOT:

Bit6
INT2_SIG_S
TEP

Bit5
INT2_D_
TAP

Bit4
INT2_T_TA
P

Bit3
INT2_STEP

Bit2
INT2_HD

Bit1
INT2_RAISE

Bit0
INT2_SI
G_MOT

R/W
RW

Default
0x00

1, map single tap interrupt to INT2 pin
0, not map single tap interrupt to INT2 pin
1, map significant step interrupt to INT2 pin
0, not map significant step interrupt to INT2 pin
1, map double tap interrupt to INT2 pin
0, not map double tap interrupt to INT2 pin
1, map triple tap interrupt to INT2 pin
0, not map triple tap interrupt to INT2 pin
1, map step valid interrupt to INT2 pin
0, not map step valid interrupt to INT2 pin
1, map hand down interrupt to INT2 pin
0, not map hand down interrupt to INT2 pin
1, map raise hand interrupt to INT2 pin
0, not map raise hand interrupt to INT2 pin
1, map significant interrupt to INT2 pin
0, not map significant interrupt to INT2 pin

Register 0x1c (INT_MAP3)

Bit7
INT2_NO_
MOT

Bit6
INT2_FWM

Bit5
INT2_FFUL
L

Bit4
INT2_DAT
A

Bit3

Bit2

Bit1
INT2_Q_TA
P

Bit0
INT2_ANY_
MOT

R/W
RW

Default
0x00

INT2_NO_MOT:

          1, map no_motion interrupt to INT2 pin
          0, not map no_motion interrupt to INT2 pin

INT2_FWM:

INT2_FFULL:

INT2_DATA:

INT2_Q_TAP:

INT2_ANY_MOT:

1, map FIFO watermark interrupt to INT2 pin
0, not map FIFO watermark interrupt to INT2 pin
1, map FIFO full interrupt to INT2 pin
0, not map FIFO full interrupt to INT2 pin

          1, map register data ready interrupt to INT2 pin
          0, not map register data ready interrupt to INT2 pin

1, map quad tap interrupt to INT2 pin
0, not map quad tap interrupt to INT2 pin

          1, map any motion interrupt to INT2 pin
          0, not map any motion interrupt to INT2 pin

Register 0x1d (STEP_CFG0)

The information contained herein is the exclusive property of QST, and shall not be distributed,
reproduced, or disclosed in whole or in part without prior written permission of QST.

  34  /  40

Document #:  13-52-21  Title:  QMA6100 Datasheet

Rev: A

矽睿

Bit7
Bit6
STEP_INTERVAL<7:0>
STEP_INTERVAL <7:0>:

Bit5

Bit4

Bit3

Bit2

Bit1

Bit0

R/W
RW

Default
0x00

algorithm setting

Register 0x1e (STEP_CFG1)

Bit6

Bit7
NLPF_STEP<1:0>
NLPF_STEP<1:0>:
TAP_QUIET_TH<5:0>:

Bit5
TAP_QUIET<5:0>

Bit4

Bit3

Bit2

Bit1

Bit0

R/W
RW

Default
0x08

Moving Average of Step: 1/2/4/8
Tap quiet threshold selection, LSB of TAP_QUIET_TH<5:0> is 31.25mg in all full scale.

Register 0x1f

Bit6

Bit7
STEP_START_CNT<2:0>
STEP_START_CNT<2:0>:
STEP_COUNT_PEAK<2:0>:
STEP_COUNT_P2P<2:0>:

Bit5

Bit4
STEP_COUNT_PEAK<1:0>

Bit3

Bit2
STEP_COUNT_P2P<2:0>

Bit1

Bit0

R/W
RW

Default
0xA9

algorithm setting
algorithm setting
algorithm setting

Bit3
INT2_OD

Bit2
INT2_LVL

Bit1
INT1_OD

Bit0
INT1_LVL

R/W
RW

Default
0x05

Register 0x20 (INTPIN_CONF)

Bit7
DIS_PU_SE
NB

Bit6
DIS_IE_AD
0

Bit5
EN_SPI3W

Bit4
STEP_COU
NT_PEAK<
2>

DIS_PU_SENB:

DIS_IE_AD0:

EN_SPI3W:

1, disable pull-up resistor of PIN_SENB
0, enable pull-up resistor of PIN_SENB
1, disable input of AD0
0, not disable input of AD0
1, enable 3W SPI
0, 4W SPI

STEP_COUNT_PEAK<2>:
INT2_OD:

INT2_LVL:

INT1_OD:

INT1_LVL:

Definition in 0x1F<4:3>

1, open-drain for INT2 pin
0, push-pull for INT2 pin
1, logic high as active level for INT2 pin
0, logic low as active level for INT2 pin
1, open-drain for INT1 pin
0, push-pull for INT1 pin
1, logic high as active level for INT1 pin
0, logic low as active level for INT1 pin

Register 0x21 (INT_CFG)

Bit7
INT_RD_CL
R

INT_RD_CLR:

SHADOW_DIS:

DIS_I2C:

Bit6
SHADOW_
DIS

Bit5
DIS_I2C

Bit4

Bit3

Bit2

Bit1
LATCH_INT
_STEP

Bit0
LATCH_INT

R/W
RW

Default
0x0C

1, clear all the interrupts in latched-mode, when any read operation to any of registers from 0x09 to 0x0D
0, clear the related interrupts, only when read the register INT_ST (0x09 to 0x0D),
no matter the interrupts in latched-mode, or in non-latched-mode.
Reading 0x09 will clear the register 0x09 only and the others keep the status
1, disable the shadowing function for the acceleration data
0, enable the shadowing function for the acceleration data.
When shadowing is enabled, the MSB of the acceleration data is locked,
when corresponding LSB of the data is reading.
This can ensure the integrity of the acceleration data during the reading.
The MSB will be unlocked when the MSB is read.
1: disable I2C. Setting this bit to 1 in SPI mode is recommended
0: enable I2C

LATCH_INT_STEP:  1, step related interrupt is in latch mode

0, step related interrupt is in non-latch mode
1, interrupt is in latch mode
0, interrupt is in non-latch mode

LATCH_INT:

Register 0x22

Bit7
RAISE_WAKE_DIFF_TH<1:0>

Bit6

Bit5
RAISE_WAKE_SUM_TH<5:0>

Bit4

RAISE_WAKE_DIFF_TH<1:0>: Threshold = 0 ~ 31.5 (LSB 0.5)
RAISE_WAKE_SUM_TH<5:0>:

Bit3

Bit2

Bit1

Bit0

R/W
RW

Default
0xD8

The information contained herein is the exclusive property of QST, and shall not be distributed,
reproduced, or disclosed in whole or in part without prior written permission of QST.

  35  /  40

Document #:  13-52-21  Title:  QMA6100 Datasheet

Rev: A

矽睿

0
1
2
3
4
5
6
7
8
9
10
Default

Register 0x23

0.2
0.3
0.4
0.5
0.6
0.7
0.8
0.9
1.0
1.1
1.2
0.2

Bit6

Bit5

Bit7
HD_Z_TH<2:0>
HD_Z_TH<2:0>:
HD_X_TH<2:0>:
RAISE_WAKE_DIFF_TH<3:2>:  Threshold = 0 ~ 31.5 (LSB 0.5)

hand down z threshold, 0~7
hand down x threshold, 0~7

Bit4
HD_X_TH<2:0>

Register 0x24

Bit3

Bit2

Bit1
RAISE_WAKE_DIFF_TH<3:2>

Bit0

R/W
RW

Default
0x7C

Bit4

Bit3

Bit2

Bit1

Bit0

Bit6

Bit7
RAISE_WAKE_TIMEOUT_TH<7:0>
RAISE_WAKE_TIMEOUT_TH<7:0>:

Bit5

Raise_wake_timeout_th[11:0] * ODR period = timeout count

Register 0x25

Bit6

Bit7
RAISE_WAKE_PERIOD<7:0>
RAISE_WAKE_PERIOD<7:0>:

Register 0x26

Bit5

Bit4

Bit3

Bit2

Bit1

Bit0

Raise_wake_period[10:0] * ODR period = wake count

Bit6
RAISE_WAKE_PERIOD<10:8>

Bit5

Bit7
RAISE_MODE
RAISE_MODE:
RAISE_WAKE_PERIOD<10:8>:
RAISE_WAKE_TIMEOUT_TH<11:8>:

0:raise wake function, 1:ear-in function

Raise_wake_period[10:0] * ODR period = wake count
Raise_wake_timeout_th[11:0] * ODR period = timeout count

Bit4

Bit3
RAISE_WAKE_TIMEOUT_TH<11:8>

Bit2

Bit1

Bit0

R/W
RW

R/W
RW

R/W
RW

Default
0x00

Default
0x00

Default
0x02

Register 0x27 (OS_CUST_X)

Bit6

Bit7
OS_CUST_X<7:0>
OS_CUST_X<7:0>:

Bit5

Bit4

Bit3

Bit2

Bit1

Bit0

R/W
RW

Default
0x00

offset calibration of X axis for user, the LSB depends on full-scale of the device which is 3.9mg in 2g range,
7.8mg in 4g range, 15.6mg in 8g range, 31.2mg in 16g, and 62.5mg in 32g

Register 0x28 (OS_CUST_Y)

Bit6

Bit7
OS_CUST_Y<7:0>
OS_CUST_Y<7:0>:

Bit5

Bit4

Bit3

Bit2

Bit1

Bit0

R/W
RW

Default
0x00

offset calibration of Y axis for user, the LSB depends on full-scale of the device which is 3.9mg in 2g range,
7.8mg in 4g range, 15.6mg in 8g range, 31.2mg in 16g, and 62.5mg in 32g

Register 0x29 (OS_CUST_Z)

Bit6

Bit7
OS_CUST_Z<7:0>
OS_CUST_Z<7:0>:

Bit5

Bit4

Bit3

Bit2

Bit1

Bit0

R/W
RW

Default
0x00

offset calibration of Z axis for user, the LSB depends on full-scale of the device which is 3.9mg in 2g range,
7.8mg in 4g range, 15.6mg in 8g range, 31.2mg in 16g, and 62.5mg in 32g

Register 0x2a (RAISE_WAKE_SUM_TH RAISE_WAKE_DIFF_TH)

Bit7
TAP_QUIET

Bit6
TAP_SHOC
K

Bit5
TAP_DELA
Y

Bit4
TAP_EARIN

Bit3

Bit2
TAP_DUR<2:0>

Bit1

Bit0

R/W
RW

Default
0x05

TAP_QUIET:
TAP_SHOCK:

1: Tap quiet time = 30ms    0: Tap quiet time = 20ms
1: Tap shock time = 50ms    0: Tap shock time = 75ms

The information contained herein is the exclusive property of QST, and shall not be distributed,
reproduced, or disclosed in whole or in part without prior written permission of QST.

  36  /  40

Document #:  13-52-21  Title:  QMA6100 Datasheet

Rev: A

矽睿

TAP_DELAY_Y:

0 : Triple tap interrupt would not wait for quadruple tap result.
1 : Triple tap interrupt would wait for quadruple tap result.

If quadruple tap is not toggle, triple tap would toggle after tap duration time finish.

TAP_EARIN:

TAP_DUR<2:0>:

1 : Tap enable would be related with EARIN_FLAG (reg 0x0B<1>).
If EARIN_FLAG is low, tap detection will be disabled.
If EARIN_FLAG is high, tap detection is enabled by reg 0x16.
0 : Tap detection is enabled by reg 0x16.
Tap duration time selection
000: 100mS
001: 150mS
010: 200mS
011: 250mS
100: 300mS
101: 400mS
110: 500mS
111: 700mS

Register 0x2b (RAISE_WAKE_DIFF_TH HD_X_TH HD_Z_TH)

Bit7
Bit6
TAP_IN_SEL<1:0>

Bit5
TAP_SHOCK_TH<5:0>

Bit4

Bit3

Bit2

Bit1

Bit0

R/W
RW

Default
0xCD

TAP_IN_SEL<1:0>:  Tap Detector Input Selection
0 : X-axis
1 : Y-axis
2 : Z-axis
3 : (X^2 + Y^2 + Z^2)^0.5

TAP_SHOCK_TH:  Tap shock threshold selection, LSB of TAP_SHOCK_TH<5:0> is 31.25mg in all full scale.

Register 0x2c (MOT_CONF0)

Bit6

Bit7
NO_MOT_DUR<5:0>
NO_MOT_DUR<5:0>:

ANY_MOT_DUR<1:0>:

Bit5

Bit4

Bit3

Bit2

Bit1
ANY_MOT_DUR<1:0>

Bit0

R/W
RW

Default
0x00

no motion interrupt will be triggered when slope < NO_MOT_TH for the times which defined by NO_MOT_DUR<5:0>
Duration = (NO_MOT_DUR<3:0> + 1) * 1s, if NO_MOT_DUR<5:4> =b00
Duration = (NO_MOT_DUR<3:0> + 4) * 5s, if NO_MOT_DUR<5:4> =b01
Duration = (NO_MOT_DUR<3:0> + 10) * 10s, if NO_MOT_DUR<5:4> =b1x
any motion interrupt will be triggered when slope > ANY_MOT_TH for (ANY_MOT_DUR<1:0> + 1) samples

Register 0x2d (MOT_CONF1)

Bit7
Bit6
NO_MOT_TH<7:0>
NO_MOT_TH<7:0>:

Bit5

Bit4

Bit3

Bit2

Bit1

Bit0

Threshold of no-motion interrupt. The threshold definition is as following

          TH= NO_MOT_TH<7:0> * 16 * LSB

Register 0x2e (MOT_CONF2)

Bit6

Bit7
ANY_MOT_TH<7:0>
ANY_MOT_TH<7:0>:

Bit5

Bit4

Bit3

Bit2

Bit1

Bit0

Threshold of any motion interrupt. The threshold definition is as following
ANY_MOT_IN_SEL = 0 : Threshold = ANY_MOT_TH<7:0> * 16LSB
ANY_MOT_IN_SEL = 1 : Threshold = ANY_MOT_TH<7:0> * 32LSB
ANT_MOT_IN_SEL is 0x2F<6>.

R/W
RW

Default
0x00

R/W
RW

Default
0x00

Register 0x2f (MOT_CONF3)

Bit7
RFF_BP_LP
F

RFF_BP_LP:

Bit6
ANY_MOT
_IN_SEL

Bit5
SIG_MOT_TPROOF<1:0>

Bit4

Bit3
SIG_MOT_TSKIP<1:0>

Bit2

Bit1

Bit0
SIG_MOT_
SEL

R/W
RW

Default
0x00

1: Data of register acceleration XYZ (0x01 ~ 0x06) and FIFO (0x3F) would bypass LPF.
0: Data of register file acceleration XYZ (0x01 ~ 0x06) and FIFO (0x3F) would be filtered by LPF.

ANY_MOT_IN_SEL:

0: Any-motion Input is Slope.
1: Any-motion Input is Acceleration, it could detect high-g.

SIG_MOT_TPROOF<1:0>:

SIG_MOT_TSKIP<1:0>:

00, T_PROOF=0.25s
01, T_PROOF=0.5s
10, T_PROOF=1s
11, T_PROOF=2s
00, T_SKIP=1.5s

The information contained herein is the exclusive property of QST, and shall not be distributed,
reproduced, or disclosed in whole or in part without prior written permission of QST.

  37  /  40

SIG_MOT_SEL:

Register 0x30

Bit7
MO_BP_LP
F

MO_BP_LPF:

STEP_BP_LPF:

Bit7
SELFTEST_
BIT

SELFTEST_BIT:

SELFTEST_SIGN:

STEP_BP_AXIS<1:0>:

Document #:  13-52-21  Title:  QMA6100 Datasheet

Rev: A

矽睿

01, T_SKIP=3s
10, T_SKIP=6s
11, T_SKIP=12s
1, select significant motion interrupt
0, select any motion interrupt

Bit6
STEP_BP_L
PF

Bit5
TAP_RST_
N

Bit4

Bit3

Bit2
NO_MOT_
RST_N

Bit1
SIG_MOT_
RST_N

Bit0
ANY_MOT
_RST_N

R/W
RW

Default
0x1F

1: Input of any motion, sig motion and no motion would bypass LPF.
0: Input of any motion, significant motion and no motion would be filtered by LPF.
1: Input of step counter, raise wake, and tap detector would bypass LPF.
0: Input of step counter, raise wake, and tap detector would be filtered by LPF.
0, Reset tap detector. After reset, user should write 1 back.

TAP_RST_N:
NO_MOT_RST_N:  0, Reset no motion detector. After reset, user should write 1 back.
SIG_MOT_RST_N:  0, Reset significant motion detector. After reset, user should write 1 back.
ANY_MOT_RST_N: 0, Reset any motion detector. After reset, user should write 1 back.

Register 0x31

Bit7
FIFO_WTMK_LVL<7:0>

Bit6

Bit5

Bit4

Bit3

Bit2

Bit1

Bit0

R/W

Default
0x00

FIFO_WTMK_LVL<7:0>:  defines FIFO water mark level. Interrupt will be generated, when the number of entries in the FIFO exceeds FIFO_WTMK_LVL<7:0>.

When the value of this register is changed, the FIFO_FRAME_COUNTER in 0x0E is reset to 0.

Register 0x32 (ST)

Bit6

Bit5

Bit4

Bit3

Bit2
SELFTEST_
SIGN

Bit0
Bit1
STEP_BP_AXIS<1:0>

R/W
RW

Default
0x00

1, self-test enabled. When self-test enabled, a delay of 3ms is necessary for the value settling.
0, normal
1, set self-test excitation positive
0, set self-test excitation negative
11, bypass Z axis, use only X and Y axes data for step counter algorithm
10, bypass Y axis, use only X and Z axes data for step counter algorithm
01, bypass X axis, use only Y and Z axes data for step counter algorithm
00, use all of 3 axes data for step counter algorithm

Register 0x34 (Y_TH YZ_TH_SEL)

Bit6

Bit7
YZ_TH_SEL<2:0>
Y_TH: -16 ~ 15 (m/s2)
YZ_TH_SEL<2:0>
0
1
2
3
4
5
6
7

Bit5

Bit4
Y_TH<4:0>

Bit3

Bit2

Bit1

Bit0

R/W
RW

Default
0x9D

UNIT (m/s2)
7.0
7.5
8.0
8.5
9.0
9.5
10.0
10.5

Register 0x35 (RAISE_WAKE_PERIOD)
Bit6

Bit5

Bit7
Z_TH<3:0>

Bit4

Bit3
X_TH<3:0>

Bit2

Bit1

Bit0

R/W
RW

Default
0x66

X_TH[3:0]: 0 ~ 7.5
Z_TH[3:0] : -8 ~ 7

Register 0x36 (SR)

Bit7
SOFT_RESET
SOFT_RESET:

Bit6

Bit5

Bit4

Bit3

Bit2

Bit1

Bit0

R/W
RW

Default
0x00

0xB6, soft reset all of the registers. After soft-reset, user should write 0x00 back

Register 0x3e (FIFO_CFG0)

The information contained herein is the exclusive property of QST, and shall not be distributed,
reproduced, or disclosed in whole or in part without prior written permission of QST.

  38  /  40

Document #:  13-52-21  Title:  QMA6100 Datasheet

Rev: A

矽睿

Bit7
Bit6
FIFO_MODE<1:0>

Bit5
RAISE_XYZ_SW<2:0>

Bit4

Bit3

Bit2
FIFO_EN_Z

Bit1
FIFO_EN_Y

Bit0
FIFO_EN_X

R/W
RW

Default
0x07

FIFO_MODE<1:0> : FIFO_MODE<1:0>: FIFO_MODE defines FIFO mode of the device. Settings as following

FIFO_MODE<1:0>
11
10
01
00

MODE
FIFO
STREAM
FIFO
BYPASS

RAISE_XYZ_SW<2:0> is x/y/z axis switcher, default setting is “0: XYZ” and below is the detail configuration. Both raise wake and ear in/out can use this function.

0x3E[5:3]

0

1

2

3

4

5

6

7

X

X

X

Y

Y

Z

Z

X

X

Y

Y

Z

X

Z

X

Y

Y

Y

Z

Z

Y

Z

X

Y

X

Z

Z

0x3E[2:0]: User can select the acceleration data of which axis to be stored in the FIFO. This configuration can be done by setting FIFO_CH, where ‘111b’ for x-, y-, and
z-axis, ‘001b’ for x-axis only, ‘010b’ for y-axis only, ‘100b’ for z-axis only.

Register 0x3f (FIFO_DATA)

Bit7
FIFO_DATA<7:0>

Bit6

Bit5

Bit4

Bit3

Bit2

Bit1

Bit0

R/W
R

Default
0x00

FIFO_DATA<7:0>: FIFO read out data. User can read out FIFO data through this register. Data format depends on the setting of FIFO_CH (0x3e<2:0>).
When the FIFO data is the LSB part of acceleration data, and if FIFO is empty, then FIFO_DATA<0> is 0. Otherwise if FIFO is not empty and the data is effective,
FIFO_DATA<0> is 1 when reading LSB of acceleration.

ORDERING INFORMATION

Ordering Number

Temperature Range

Package

Packaging

QMA6100

-40℃~85℃

LGA-12

Tape and Reel: 5k pieces/reel

FIND OUT MORE

For more information on QST’s Accelerometer Sensors contact us at 86-21-69517300.

The application circuits herein constitute typical usage and interface of QST product. QST does not provide warranty or assume
liability of customer-designed circuits derived from this description or depiction.

The information contained herein is the exclusive property of QST, and shall not be distributed,
reproduced, or disclosed in whole or in part without prior written permission of QST.

  39  /  40

QST reserves the right to make changes to improve reliability, function or design. QST does not assume any liability arising out
of the application or use of any product or circuit described herein; neither does it convey any license under its patent rights
nor the rights of others.

ISO9001 : 2015

China Patents 201510000399.8, 201510000425.7, 201310426346.3, 201310426677.7, 201310426729.0, 201210585811.3 and
201210553014.7 apply to the technology described.


