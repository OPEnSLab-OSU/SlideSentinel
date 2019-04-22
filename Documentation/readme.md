#Slide Sentinel Project Specification Guide

## Base

Specification | Status 
--- | --- | ---
Power Budget | Nominally draws 150-160mA 
Remote Configuration | Hub can recieve remote configuration data to control the satcom upload cycle time, set verbosity scale, and force a satcom uploads.
Deployment Duration | System must last for 1 month
Range | Base unit will be elevated with reference to the node, one mile distance apart, concrete base.

## Rover

Specification | Status
--- | --- | ---
Power Budget | Nominally draws X mA 
Solar Power | Solar power charger supplies ~20mA
Positional Accuracy | Rover must generate sub-centimeter positional calculations at least once a day, with partial tree cover (RTK fix with RTK ratio of 10)
Deployment Duration | System must last 1 month at 15 minute wake cycles
Accelerometer | Sudden orientation shifts must wake Rover unit up from sleep and trigger positional calculations.

## Rover Mounting Stake

