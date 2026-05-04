# LED Pattern Designer

A comprehensive GUI tool for designing, visualizing, and exporting WS2812 LED patterns for the LED-Controller-OS.

## Features

- **Interactive Pattern Editor**: Click on the grid to set individual LED colors
- **Real-Time Visualization**: See your patterns with accurate RGB color display
- **Color Interpolation**: Define keypoints and automatically interpolate colors between them
- **Gradient Tools**: Create smooth gradients with a single click
- **Animation Preview**: Preview how your pattern animates with real-time playback
- **Multiple LED Formats**: Support for RGB, BGR, GRB, RGBW, WRBG, WRGB, GRBW, WGRB, WBGR
- **Flexible Configuration**: 
  - LED density: 30, 60, 120 LEDs/meter
  - Custom strip length and total LED count
  - Adjustable frame rate and offset jump
- **Binary Export**: Export patterns to binary format compatible with LED-Controller-OS
- **Pattern Save/Load**: Save and load patterns as JSON for future editing

## Installation

### Prerequisites

- Python 3.9+
- tkinter (usually included with Python)
- The `dfile` tool built from TestPrograms/DataFile

### Building dfile Tool

```bash
cd /root/LED-Controller-OS/TestPrograms/DataFile
make
```

### Running Pattern Designer

```bash
python3 pattern_designer.py
```

Or from the Helper-Tools directory:

```bash
./pattern_designer.py
```

## Usage Guide

### 1. Set Up Your Strip Configuration

On the right panel under "Settings":

- **LED Format**: Choose your LED color format (RGB, RGBW, etc.)
- **Density**: Select 30, 60, or 120 LEDs per meter
- **Strip Length**: Enter total physical length in centimeters
- **Frame Delay**: Time in milliseconds between animation frames
- **Offset Jump**: How many pixels to advance per frame
- **Total LEDs**: Automatically calculated based on strip length and density

### 2. Create Your Pattern

#### Method 1: Direct Color Assignment
1. Use the color picker on the left to select a color
   - Adjust R, G, B sliders or click the color box for a full color picker
2. Click on the pattern grid to place the color on LEDs
3. Continue clicking to build your pattern

#### Method 2: Keypoint Interpolation with Splines
1. Click on specific LED positions to set keypoints
2. Click the "Interpolate" button to smoothly blend between keypoints using Catmull-Rom splines
3. The GUI will automatically fill in intermediate colors with smooth curves
4. This creates natural-looking color transitions (better than linear interpolation)

#### Method 3: Gradient
1. Select your desired color
2. Click "Gradient" to create a smooth gradient from black to that color

#### Method 4: Load Existing Pattern
1. Click "Load Pattern" and select a previously saved JSON file
2. Edit as needed

### 3. Preview Your Animation

1. Click "Start Animation" to begin playback
2. The preview panel shows how the pattern flows across the strip
3. Adjust "Frame Delay" and "Offset Jump" to control animation speed and smoothness
4. Click "Reset Frame" to go back to frame 0

### 4. Save and Export

#### Save Pattern (JSON)
- Click "Save Pattern" to save your work as JSON
- Includes all LED values and keypoint information
- Can be loaded later for editing

#### Export to Binary
- Click "Export to Binary" to create the binary file
- Automatically uses the `dfile` tool to create a compatible binary file
- The file includes:
  - Magic number: `0x5053544E` ("PSTN")
  - Pattern data field: signature "PATT"
  - All 32-bit LED values in your selected format

## Animation Concepts

### Offset Jump
The `offset_jump` parameter controls how many pixels the display window moves per frame:

- **offset_jump = 1**: Show pixels [0-149], then [1-150], then [2-151], etc. (smooth scrolling)
- **offset_jump = 5**: Jump 5 pixels per frame (more discrete movement)
- **offset_jump = 150**: Jump by entire window (pop to next section)

### Pattern Looping
When the read pointer reaches the end of the pattern data, it wraps back to the beginning without concatenation. This allows:

- **Moving patterns**: Offset jump creates scrolling effect
- **Repeating sections**: Pattern can represent one cycle of animation
- **Non-linear loops**: No requirement for pattern start/end to match

### Total Pattern Size
The total pattern size is determined by:
- Calculation: `num_leds = (density * strip_length_cm) / 100`
- Example: 60 LED/m × 100 cm = 600 LEDs per pattern cycle

## File Formats

### JSON Pattern Format

```json
{
  "leds": [0, 16711680, 255, ...],
  "keypoints": {
    "0": [255, 0, 0, 0],
    "100": [0, 255, 0, 0],
    "200": [0, 0, 255, 0]
  },
  "settings": {
    "format": "RGB",
    "density": 60,
    "strip_length_cm": 100
  }
}
```

### Binary Pattern Format

Created with `dfile` tool:

```
Header (12 bytes)
├─ Magic: 0x5053544E ("PSTN")
├─ Field 1 (tim): Frame timing in milliseconds
│  ├─ Signature: "tim"
│  ├─ Length: 2 bytes (16-bit unsigned)
│  └─ Data: Frame delay in ms (big-endian)
├─ Field 2 (jmp): Offset jump per frame
│  ├─ Signature: "jmp"
│  ├─ Length: 2 bytes (16-bit unsigned)
│  └─ Data: Pixels to advance per frame (big-endian)
├─ Field 3 (dat): LED pattern data
│  ├─ Signature: "dat"
│  ├─ Length: num_leds × 4 bytes
│  └─ Data: 32-bit RGB(W) values in specified format (big-endian)
```

## Interpolation Algorithm

The pattern designer uses **Catmull-Rom cubic splines** for smooth color interpolation between keypoints. This creates natural-looking color transitions that are superior to simple linear interpolation.

### How It Works

1. Set color keypoints at different LED positions
2. Click "Interpolate" to calculate smooth transitions
3. The algorithm uses the surrounding keypoints to calculate smooth curves
4. Color values are automatically clamped to 0-255 to prevent overflow

### Why Catmull-Rom?

- **Smooth curves**: Creates visually pleasing gradients
- **No overshoot at endpoints**: Values stay within reasonable bounds
- **Natural transitions**: Mimics hand-drawn curves
- **No external dependencies**: Built-in implementation



Each 32-bit value represents one LED in the selected format:

| Format | Byte Layout | Use Case |
|--------|------------|----------|
| RGB | 0x00RRGGBB | Standard WS2812 |
| BGR | 0x00BBGGRR | Some WS2812 variants |
| GRB | 0x00GGRBRR | Common WS2812 actual layout |
| RGBW | 0xWWRRGGBB | RGBW LEDs with white channel |
| GRBW | 0xWWGGBBRR | GRBW format |
| WRGB | 0xRRGGBBWW | White-first RGBW |
| WRBG | 0xRRBBGGWW | White-first RBGW |
| WGRB | 0xGGBBRRWW | White-first GBRW |
| WBGR | 0xBBGGRRWW | White-first BGRW |

## Troubleshooting

### dfile tool not found
**Error**: "Could not find dfile tool"

**Solution**: Build the dfile tool first:
```bash
cd /root/LED-Controller-OS/TestPrograms/DataFile
make
```

### Color picker not working
**Issue**: Color sliders don't respond

**Solution**: Make sure to click in the entry field to update the value, or use the color box dialog

### Animation is too slow/fast
**Adjustment**: Change "Frame Delay" (milliseconds) or "Offset Jump" (pixels/frame)

### Pattern wraps incorrectly
**Ensure**: Your pattern data doesn't need to connect at the end. The offset jump creates natural looping.

## Advanced Tips

1. **Smooth gradients**: Use interpolation with multiple keypoints
2. **Complex patterns**: Combine gradients and direct color assignment
3. **Testing animations**: Adjust offset jump and frame delay to see different effects
4. **Pattern length**: Total pattern can be larger than physical strip for variety
5. **Performance**: Device will smoothly play patterns up to several thousand LEDs

## Device Integration

Once you have a `.bin` file, upload it to your LED-Controller-OS device:

```bash
# Using filesync.py
python3 filesync.py
# Then use Upload -> to send the pattern file to device
```

Your device code should:
1. Load the pattern from the data file
2. Read the timing parameter from the "tim" field (frame delay in ms)
3. Read the offset jump from the "jmp" field (pixels to advance per frame)
4. Read the LED data from the "dat" field
5. Each frame:
   - Read `num_visible_leds` pixels starting from current offset
   - Display to LED strip
   - Increment offset by the jump value
   - Wait for frame delay
   - Wrap offset back to 0 when reaching end of pattern

### Reading Binary Pattern File

Example device code flow:
```cpp
// Pseudocode
uint32_t pattern_data[num_leds];  // LED values from "dat" field
uint16_t frame_delay_ms;           // From "tim" field
uint16_t offset_jump;              // From "jmp" field

uint32_t current_offset = 0;

while (animating) {
    // Display current window of LEDs
    for (int i = 0; i < num_visible_leds; i++) {
        uint32_t led_index = (current_offset + i) % num_leds;
        show_led(i, pattern_data[led_index]);
    }
    
    // Advance and wrap
    current_offset = (current_offset + offset_jump) % num_leds;
    delay_ms(frame_delay_ms);
}
```

## License

Same as LED-Controller-OS project
