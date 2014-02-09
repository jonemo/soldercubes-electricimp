/*
 ElectricImp device code for imp-based serial communication bridge between
 a Soldercube assembly and the Soldercubes graphical user interface.
 
 Author: Jonas Neubert
 Date: August 2013
 
 */

local rxLEDToggle = 1;  // These variables keep track of rx/tx LED toggling status
const BAUD_RATE = 57600; // 57600 is what the cube main boards are set to

agent.on("incomingbytes", function(bytes)
{
  try 
  { 
    if (typeof bytes != "array") throw "Incoming data is not an array.";
  
    foreach (b in bytes) {
      hardware.uart57.write(b.tochar());
    }
  }
  catch (ex)
  {
    server.log(ex);
    return;
  }

  server.log("Imp sent " + bytes.len() + " characters to serial bus.");
  toggleRxLED();
});

function initUart()
{
    hardware.configure(UART_57);    // Using UART on pins 5 and 7
    hardware.uart57.configure(BAUD_RATE, 8, PARITY_NONE, 1, NO_CTSRTS); 
}

function initLEDs()
{
    // LEDs are on pins 8 and 9 on the imp Shield
    // They're both active low, so writing the pin a 1 will turn the LED off
    hardware.pin8.configure(DIGITAL_OUT_OD_PULLUP);
    hardware.pin8.write(1);
}

// This function turns an LED on/off quickly on pin 8.
function toggleRxLED()
{
    rxLEDToggle = rxLEDToggle ? 0 : 1;
    if (!rxLEDToggle)
    {
        imp.wakeup(0.1, toggleRxLED.bindenv(this)); // if we're turning the LED on, set a timer to call this function again (to turn the LED off)
    }
    hardware.pin8.write(rxLEDToggle);   // RX LED is on pin 8 (active-low)
}

// function for polling the serial port (calls itself via wakeup)
function readSerial() 
{
    local resultChar = hardware.uart57.read();
    local result = [];
    while (resultChar != -1 && result.len() < 1024)
    {
        result.append(resultChar);
        resultChar = hardware.uart57.read();
    }
    
    if (result.len() != 0) 
    {
      server.log("Imp read " + result.len() + " characters from serial bus.");
      agent.send("outgoingbytes", result);
      // we just read something and called the agent, lets wait a little longer until we check again
      imp.wakeup(1.0, readSerial);
    }
    else
    {
      // we read nothing now so we didn't call the agent, so let's check again shortly
      imp.wakeup(0.1, readSerial);
    }
}

// This'll configure our impee. It's name is "LED Mood Light", and it has both an input and output to be connected:
imp.configure("Robot Ecology Imp Module", [], []);

// Initialize the UART, called just once
initUart(); 

// Initialize the LEDs, called just once
initLEDs(); 

// trigger first readSerial() call (will keep calling itself via wakeup)
readSerial();

server.log("Done starting.");