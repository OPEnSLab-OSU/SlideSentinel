# Project Loom: Library Files

This folder is where all of the Loom Library files live.

If you are looking for the comprehensive readme that used be here, it is now one directory up, now found [here](https://github.com/OPEnSLab-OSU/InternetOfAg)

# PCB Design
For PCB design specifically for the RTK device:
  A 50 ohm impedence is required for the RF Signal Path. Vias can cause the impedence to
  slightly decrease, decreasing the quality of the signal. Thus, it is important to try and
  keep a direct line on the same level for the PCB design. The RF Ground and Power can use
  Viasses. Also another note is to to not use 90 degree angles, use two 45 deg angles or a 
  round bend for the signal path.
  
  The pin # is 17 and pin name is RFIN. 
