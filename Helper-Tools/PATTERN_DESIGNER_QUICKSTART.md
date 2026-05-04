# Pattern Designer - Quick Start Guide

## Installation

```bash
cd /root/LED-Controller-OS/Helper-Tools
python3 pattern_designer.py
```

## Basic Workflow (5 minutes)

### Step 1: Launch the App
```bash
python3 pattern_designer.py
```

### Step 2: Configure Your Strip
1. **LED Format**: Choose RGB or RGBW based on your LED type
2. **Density**: Select 30, 60, or 120 LEDs/meter
3. **Strip Length**: Enter length in centimeters
4. Leave other settings at defaults for now

### Step 3: Create a Simple Pattern
1. Click the color display box (upper left) to pick a color
2. Click on the pattern grid to paint LEDs
3. Try different colors by selecting them and clicking more areas

### Step 4: Try Interpolation
1. Click on LED 0 and set it to RED
2. Click on LED 149 and set it to BLUE
3. Click the "Interpolate" button
4. Watch the smooth gradient appear!

### Step 5: Preview Animation
1. Click "Start Animation"
2. Adjust "Frame Delay (ms)" to speed up/slow down
3. Adjust "Offset Jump (px)" to change movement distance
4. Click "Stop Animation" when done

### Step 6: Export Pattern
1. Click "Export to Binary"
2. Choose where to save (e.g., `my_pattern.bin`)
3. The tool creates a binary file ready for your device

## Common Tasks

### Create a Rainbow
1. Set keypoints:
   - LED 0: Red
   - LED 50: Green
   - LED 100: Blue
2. Click "Interpolate"

### Fading Pulse
1. Set keypoint at center
2. Set color to full brightness there
3. Set keypoints at edges to black (0,0,0)
4. Interpolate

### Scrolling Chase
1. Create a narrow bright section (5-10 LEDs)
2. Keep rest dark
3. Increase "Offset Jump" to 5-10 pixels
4. Lower "Frame Delay" to 30-50ms

## LED Format Reference

| Format | Use Case |
|--------|----------|
| RGB | Most WS2812 strips |
| BGR | Some WS2812 variants |
| GRB | Common actual WS2812 layout |
| RGBW | RGBW LEDs with white channel |

Not sure? Start with **RGB** - it's most common for WS2812.

## Density Reference

| Density | Spacing | Use Case |
|---------|---------|----------|
| 30 LED/m | 33mm | Sparse strips |
| 60 LED/m | 16.7mm | Standard (most common) |
| 120 LED/m | 8.3mm | Dense strips |

## Animation Parameters

| Parameter | Effect | Typical Range |
|-----------|--------|---------------|
| Frame Delay | Time per frame | 20-200 ms |
| Offset Jump | Pixels to move | 1-20 pixels |

- **Small offset_jump (1-2)**: Smooth, slow scrolling
- **Large offset_jump (10+)**: Fast, discrete jumps

## File Locations

When exporting, your binary file contains:

```
Pattern File (.bin)
├─ Timing (tim): Frame delay in milliseconds
├─ Offset (jmp): Pixels per frame
└─ Data (dat): All LED colors (4 bytes each)
```

## Troubleshooting

**"dfile tool not found"**
- Build it first: `cd TestPrograms/DataFile && make`

**Pattern looks weird after interpolation**
- Make sure you have at least 2 keypoints set
- They should be at different positions

**Animation doesn't look right**
- Increase "Frame Delay" for slower animation
- Decrease "Offset Jump" for smoother movement

## Next Steps

1. Save your pattern (JSON) for later editing
2. Export to binary for your device
3. Upload to LED-Controller-OS device using `filesync.py`
4. Enjoy your custom animations!

## Tips & Tricks

- **Smooth gradients**: Use lots of keypoints for silky transitions
- **Color picker**: Click the color box for a full color picker dialog
- **Clear pattern**: Use "Clear All" to start fresh
- **Keyboard**: Press Tab to move between controls

## Support

See `PATTERN_DESIGNER_README.md` for detailed documentation.
