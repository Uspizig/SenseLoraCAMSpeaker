#include "Adafruit_VL53L0X.h"
#include "USB.h"
const byte VL53LOX_InterruptPin = 12;
const byte VL53LOX_ShutdownPin = 11;
volatile byte VL53LOX_State = LOW;
Adafruit_VL53L0X lox = Adafruit_VL53L0X();
USBCDC USBSerial;

#define SDA 5
#define SCL 6

void setup() {
  USBSerial.begin(115200);USB.begin();

  // wait until serial port opens for native USB devices
  delay(2000);
  USBSerial.println(F("VL53L0X API Interrupt Ranging example\n\n"));

  pinMode(VL53LOX_ShutdownPin, OUTPUT);
  pinMode(VL53LOX_InterruptPin, INPUT);
  attachInterrupt(digitalPinToInterrupt(VL53LOX_InterruptPin), VL53LOXISR, CHANGE);
  Wire.begin(SDA, SCL);
  // if lox.begin failes its becasue it was a warm boot and the VL53LOX is in
  // continues mesurement mode we can use an IO pin to reset the device in case
  // we get stuck in this mode
  while (!lox.begin()) {
    USBSerial.println(F("Failed to boot VL53L0X"));
    USBSerial.println("Adafruit VL53L0X XShut set Low to Force HW Reset");
    digitalWrite(VL53LOX_ShutdownPin, LOW);
    delay(100);
    digitalWrite(VL53LOX_ShutdownPin, HIGH);
    USBSerial.println("Adafruit VL53L0X XShut set high to Allow Boot");
    delay(100);
  }

  // Second Parameter options are VL53L0X_GPIOFUNCTIONALITY_OFF,
  // VL53L0X_GPIOFUNCTIONALITY_THRESHOLD_CROSSED_LOW,
  // VL53L0X_GPIOFUNCTIONALITY_THRESHOLD_CROSSED_HIGH,
  // VL53L0X_GPIOFUNCTIONALITY_THRESHOLD_CROSSED_OUT,
  // VL53L0X_GPIOFUNCTIONALITY_NEW_MEASURE_READY

  USBSerial.println("Set GPIO Config so if range is lower the LowThreshold "
                 "trigger Gpio Pin ");
  lox.setGpioConfig(VL53L0X_DEVICEMODE_CONTINUOUS_RANGING,
                    VL53L0X_GPIOFUNCTIONALITY_THRESHOLD_CROSSED_LOW,
                    VL53L0X_INTERRUPTPOLARITY_LOW);

  // Set Interrupt Treashholds
  // Low reading set to 50mm  High Set to 100mm
  FixPoint1616_t LowThreashHold = (50 * 65536.0);
  FixPoint1616_t HighThreashHold = (100 * 65536.0);
  USBSerial.println("Set Interrupt Threasholds... ");
  lox.setInterruptThresholds(LowThreashHold, HighThreashHold, true);

  // Enable Continous Measurement Mode
  USBSerial.println("Set Mode VL53L0X_DEVICEMODE_CONTINUOUS_RANGING... ");
  lox.setDeviceMode(VL53L0X_DEVICEMODE_CONTINUOUS_RANGING, true); //Debug true

  USBSerial.println("StartMeasurement... ");
  lox.startMeasurement();
}

void VL53LOXISR() {
  // Read if we are high or low
  VL53LOX_State = digitalRead(VL53LOX_InterruptPin);
  // set the built in LED to reflect in range on for Out of range off for in
  // range
}

void loop() {
  if (VL53LOX_State == LOW) {
    VL53L0X_RangingMeasurementData_t measure;
    USBSerial.print("Reading a measurement... ");
    lox.getRangingMeasurement(&measure, true); // pass in 'true' to get debug data printout!

    if (measure.RangeStatus != 4) { // phase failures have incorrect data
      USBSerial.print("Distance (mm): ");
      USBSerial.println(measure.RangeMilliMeter);
    } else {
      USBSerial.print(" out of range ");
      USBSerial.println(VL53LOX_State);
    }
    // you have to clear the interrupt to get triggered again
    lox.clearInterruptMask(false);

  } else {
    delay(10);
  }
  //VL53LOX_State = HIGH;
}
