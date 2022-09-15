# Changelog - aetrion modular VCV Rack Modules
## Chordvault
### v2.1

* Changed: implementation of the "rack timing standard" for reset
* New: Multiple options for how channels for incoming polyphonic CV are "sorted"
  * Sorted: In this mode the CVs for low gates are removed and the CVs are sorted from lowest to highest note
  * Condensed: In this mode the CVs for low gates are removed, CVs are not sorted
  * Pristine: In this mode the all CVs are left exactly as received (channels kept as they were during recording of the step)
 * Added: 0-10V range option for step CV input (selectable via right click menu)
