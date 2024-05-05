#include <Arduino.h>
#include <Microsoft_HidForWindows.h>
#include <Adafruit_NeoPixel.h>


#define NEO_PIXEL_PIN 6
#define NEO_PIXEL_LAMP_COUNT 19
#define LED_SPACING_MILLIMETERS 35
#define NEO_PIXEL_TYPE (NEO_GRB + NEO_KHZ800)
#define NEO_PIXEL_LAMP_UPDATE_LATENCY (0x04)

Adafruit_NeoPixel neoPixelLed = Adafruit_NeoPixel(NEO_PIXEL_LAMP_COUNT, NEO_PIXEL_PIN, NEO_PIXEL_TYPE);

LampAttributes* createLampAttributes(int8_t count) {
    LampAttributes* attributesArray = new LampAttributes[count];
    for (int i = 0; i < count; ++i) {
        attributesArray[i].LampId = static_cast<uint16_t>(i);
        attributesArray[i].PositionXInMillimeters = static_cast<uint16_t>((i+1) * LED_SPACING_MILLIMETERS);
        attributesArray[i].PositionYInMillimeters = 0;
        attributesArray[i].PositionZInMillimeters = 0;
        attributesArray[i].UpdateLatencyInMilliseconds = NEO_PIXEL_LAMP_UPDATE_LATENCY;
        attributesArray[i].LampPurposes = LampPurposeAccent;
        attributesArray[i].RedLevelCount = 0xFF;
        attributesArray[i].GreenLevelCount = 0xFF;
        attributesArray[i].BlueLevelCount = 0xFF;
        attributesArray[i].IntensityLevelCount = 0x01;
        attributesArray[i].IsProgrammable = LAMP_IS_PROGRAMMABLE;
        attributesArray[i].LampKey = 0x00;
    }
    return attributesArray;
}

// The Host needs to know the location of every Lamp in the LampArray (X/Y/Z position) and other metadata.
// See "26.7 LampArray Attributes and Interrogation" https://usb.org/sites/default/files/hut1_4.pdf#page=336
LampAttributes* neoPixelLedLampAttributes PROGMEM = createLampAttributes(NEO_PIXEL_LAMP_COUNT);

// All lengths in millimeters. All times in milliseconds.
Microsoft_HidLampArray lampArray = Microsoft_HidLampArray(
    NEO_PIXEL_LAMP_COUNT, 
    NEO_PIXEL_LAMP_COUNT * LED_SPACING_MILLIMETERS, 
    55, 
    1, 
    LampArrayKindPeripheral, 
    33, 
    neoPixelLedLampAttributes
);

// When the LampArray is in Autonomous-Mode, turn off the LED strip.
uint32_t lampArrayAutonomousColor = neoPixelLed.Color(0, 0, 0);

uint32_t lampArrayColorToNeoPixelColor(LampArrayColor lampArrayColor)
{
    return neoPixelLed.Color(lampArrayColor.RedChannel, lampArrayColor.GreenChannel, lampArrayColor.BlueChannel);
}

void setup()
{
    // Initialize the NeoPixel library.
    neoPixelLed.begin();
    neoPixelLed.clear();

    // Always initially in Autonomous-Mode.
    neoPixelLed.fill(lampArrayAutonomousColor, 0, NEO_PIXEL_LAMP_COUNT - 1);
    neoPixelLed.show();
}

void loop()
{
    LampArrayColor currentLampArrayState[NEO_PIXEL_LAMP_COUNT];
    bool isAutonomousMode = lampArray.getCurrentState(currentLampArrayState);

    bool update = false;

    for (uint16_t i = 0; i < NEO_PIXEL_LAMP_COUNT; i++)
    {
        // Autonomous-Mode is the Host's mechanism to indicate to the device, that the device should decide what to render.
        // The Host may do this when no application is using the LampArray, so it has nothing to render.
        // In this case, this LampArray will revert to it's default/background effect, rendering 'blue'.
        uint32_t newColor = isAutonomousMode ? lampArrayAutonomousColor : lampArrayColorToNeoPixelColor(currentLampArrayState[i]);
        if (newColor != neoPixelLed.getPixelColor(i))
        {
            neoPixelLed.setPixelColor(i, newColor);
            update = true;
        }
    }

    // Only call update on the NeoPixels when something has changed, show() takes a long time to execute.
    if (update)
    {
        // Send the updated pixel color to hardware.
        neoPixelLed.show();
    }
}