# Autonomous Sumo Bot - "USB-C"

Competition-ready autonomous sumo robot designed to detect, engage, and push opponents out of the ring while maintaining edge awareness and strategic positioning.

## Overview
![Image](https://github.com/user-attachments/assets/d008d448-8fb6-4156-8000-102e97a56e6c)

Built for the UWA ELEC3020 Embedded Systems Sumo Bot Tournament 2025, USB-C (Unidentified SumoBot - Cat) uses a circular chassis design with a strategically engineered curved ramp to destabilize opponents while maintaining superior edge detection and autonomous decision-making capabilities.

The bot operates entirely autonomously using a finite state machine architecture, making real-time tactical decisions based on infrared line sensors and ultrasonic opponent detection.

## Competition Performance

- **Tournament**: UWA ELEC3020 Sumo Bot Competition 2025
- **Design Philosophy**: Defense-first with aggressive attack capability
- **Key Innovation**: Dipped ramp design for opponent capture and lift

## Features

- **360° Defense System** - Circular chassis provides uniform edge detection from all angles
- **Intelligent State Machine** - Five operational modes (Defense, Attack, Search 1, Search 2, Blind)
- **Curved Ramp Mechanics** - Two-stage interaction: capture phase (dip traps opponent) + lift phase (upward curve destabilizes)
- **Differential Drive** - Two-wheel system enables on-the-spot rotation for maximum maneuverability
- **Dual Battery System** - Separate power for motors and logic to prevent voltage sag during high-load situations

## Tech Stack

**Hardware:**
- **Microcontroller**: ESP32 TTGO board
- **Motors**: 2x DC motors with 1.6A stall current (differential drive)
- **Motor Driver**: L298N (handles both motors)
- **Line Sensors**: 5x TCRT5000 IR reflectance sensors with adjustable potentiometers (25mm sensing distance)
- **Opponent Detection**: 1x HC-SR04 ultrasonic sensor
- **Power System**: 
  - 2x 7.4V 12000mAh LiPo batteries (one for motors, one for electronics)
  - Buck converter (7.4V → 5V for TTGO)
- **Additional**: Ball caster (rear stabilization)

**Software:**
- C++ (Arduino framework)
- State machine architecture
- Real-time sensor fusion

**Mechanical:**
- 3D-printed PETG chassis (Fusion 360 CAD design)
- Circular design: ~20cm diameter
- Weight: <1kg

## Design Philosophy

### Circular Chassis
The round design provides **defense from every direction** rather than optimizing for frontal attacks only. The curved edge acts like a snow plow - deflecting opponents to the sides rather than engaging in direct push battles, while the focused "knife's edge" increases the probability of slipping under opponents from any angle.

### Curved Ramp with Dip
Unlike conventional straight ramps that lift opponents progressively (reducing ground traction), our ramp features a **strategic dip** that creates a two-stage interaction:

1. **Capture Phase**: The dip traps the opponent's lower edge, maintaining their ground contact while we retain maximum wheel friction
2. **Lift Phase**: The upward curve after the dip converts forward momentum into upward torque, destabilizing or flipping the opponent

This design improves our stability during collisions while providing aggressive offensive capability.

### Differential Drive
Two motors (rather than four) provide:
- **Superior maneuverability**: On-the-spot rotation in the compact arena
- **Budget efficiency**: Higher-quality motors within $50 constraint
- **Simplified control**: Fewer pins required, focus on strategy rather than synchronization
- **Weight advantage**: Lighter than four-motor designs

## Strategy Architecture

The bot operates using a **finite state machine** with interrupt-driven edge detection:

### States

**DEFENSE** (Highest Priority)
- Activates immediately when any line sensor detects black
- Front/back sensors: Full reverse + turn away from edge
- Side sensors: Arc turn (exploits curved chassis to deflect while retreating)
- Returns bot to face arena center

**ATTACK**
- Engages when opponent detected within ultrasonic range
- Full-speed advance toward target
- Continues until edge detected or opponent lost

**SEARCH 1**
- Small-arc scan (±45°) using ultrasonic sensor
- Rotates on-spot to sweep for opponents
- Transitions to SEARCH 2 if no detection

**SEARCH 2**  
- Full 360° rotation search pattern
- Slower sweep for comprehensive arena coverage

**BLIND MODE** (Planned)
- Backup strategy if ultrasonic jammed/absorbed
- Edge-following or random bouncing patterns
- Not fully implemented due to complexity

## Key Challenges Solved

**1. Motor Driver Overload**
- **Problem**: TB6612FNG burnt out (1.5A max per channel, motors draw 1.6A stall)
- **Solution**: Switched to L298N motor driver; initially used two drivers (one per motor), final design used one L298N for both motors

**2. Sensor Coverage Gaps**
- **Problem**: Circular design creates chord areas between sensors that could miss the black edge
- **Solution**: Used 5 line sensors spaced around perimeter; accepted trade-off as adding more sensors has diminishing returns

**3. Limited GPIO Pins**
- **Problem**: ESP32 TTGO has finite pins (used every available pin)
- **Solution**: Prioritized line sensors for defense over additional ultrasonic sensors; used single ultrasonic sensor instead of array

**4. Straight-Edge Target Contact**
- **Problem**: Circular design only contacted corners of rectangular opponents (failed qualification task)
- **Solution**: Added 3D-printed flat edge component to front under ultrasonic sensor for centered contact

**5. Motor Power Consistency**
- **Problem**: Different motor drivers had different efficiency characteristics
- **Solution**: Initially compensated with PWM adjustment; final design used single driver type

## Circuit Design

All components are mounted on a perfboard using the circular chassis layout:
- **5V and GND rails** run around the perimeter to minimize wire length
- **Line sensors** positioned at chassis edge (front, left-front, left-rear, right-front, right-rear)
- **Ultrasonic sensor** mounted at front for forward-facing detection
- **Motors** connect to L298N with 3 pins each (PWM, DIR1, DIR2)
- **Buck converter** steps 7.4V battery down to 5V for TTGO and sensors

## Constraints & Specifications

**Competition Rules:**
- Budget: $50 AUD
- Weight: <1kg  
- Starting size: 25cm x 25cm x 25cm maximum
- Autonomous operation (no remote control)

**Final Specifications:**
- Dimensions: ~20cm diameter (circular)
- Weight: ~800g
- Battery life: ~30+ minutes continuous operation
- Response time: <50ms (edge detection to motor action)

## Setup & Usage

**Hardware Assembly:**
1. 3D print chassis components (bottom plate, perfboard holder, top shell, ramp)
2. Mount motors in chassis slots using bolts
3. Solder components to perfboard following circuit diagram
4. Install line sensors around chassis perimeter (adjust potentiometers for calibration)
5. Mount ultrasonic sensor at front
6. Connect batteries and secure with velcro/zip ties

**Software Upload:**
```bash
# Using Arduino IDE with ESP32 board support
# 1. Install ESP32 board package
# 2. Select board: "TTGO T-Display"
# 3. Upload sketch

# Or using PlatformIO:
pio run --target upload
```

**Calibration:**
1. Place bot on white surface
2. Adjust each line sensor potentiometer until LED just turns off
3. Move bot over black tape - LED should turn on
4. Repeat for all 5 sensors
5. Test ultrasonic range (should detect objects up to ~30cm)

**Competition Mode:**
1. Power on both batteries
2. Place bot in arena (facing center)
3. Enters SEARCH mode and begins autonomous operation

## Project Structure

## Design Iterations

The robot went through multiple design revisions:
- **V1 Bottom Plate**: Basic disk with sensor holes
- **V2 Bottom Plate**: Added motor mounting slots with extrusions
- **V1 Perfboard Holder**: Separate disk (abandoned due to weight)
- **V2 Perfboard Holder**: Integrated poles that fit into bottom plate
- **V1 Ramp**: One-piece flat ramp (too passive)
- **V2 Ramp**: Separated curved ramp with strategic dip + flat edge attachment

All CAD work done in Fusion 360. Chassis printed in PETG for durability.

## Team Contributions

- **Dania Garman**: CAD design (Fusion 360), 3D printing, chassis iterations, material fabrication, project video
- **Ivan Zhao**: Documentation, testing, circuit verification, materials work, user manual
- **Beven Tsoi**: Hardware design, circuit debugging, motor control coding, PWM implementation, attack/defense logic
- **Sahil Raj**: Software architecture, sensor programming, state machine logic, circuit assembly, soldering

## Lessons Learned

- **Sensor placement is critical** - 25mm sensing distance of TCRT5000 required careful chassis height consideration
- **Current ratings matter** - Always overspec motor drivers; burnt TB6612FNG taught us this the hard way
- **Simplicity beats complexity** - Complex blind mode algorithms failed; simple search-and-attack won matches  
- **Defense > Offense** - Staying in the ring is more important than aggressive attacks
- **Pin planning is essential** - Used every ESP32 pin; pin expander would have enabled more sensors
- **Weight distribution** - Unbalanced front-heavy design (no front caster) was risky but maximized ground clearance for undercutting opponents

## Future Improvements

- [ ] Add IMU (gyroscope/accelerometer) for post-collision orientation tracking
- [ ] Implement pin expander for additional ultrasonic sensors (3-sensor array for position triangulation)
- [ ] Increase motor torque with higher-spec motors (current motors adequate but not dominant)
- [ ] Add Bluetooth module for real-time telemetry and strategy tuning
- [ ] Implement machine learning for opponent behavior prediction
- [ ] Design custom PCB to replace perfboard (cleaner, more reliable)

## Competition Rules Compliance

✅ Autonomous operation only (no remote control)  
✅ Budget: $50 AUD
✅ Weight: <1kg  
✅ Size: 20cm diameter (within 25cm x 25cm limit)  
✅ No prohibited weapons (projectiles, liquids, etc.)

## Demo Video


---

**Course**: ELEC3020 Embedded Systems  
**Institution**: University of Western Australia  
**Year**: 2025  
**Team**: Dania Garman, Ivan Zhao, Beven Tsoi, Sahil Raj  
**Instructor**: Dr. Thomas Bräunl
