# obsdfreqd

Userland CPU frequency scheduling for OpenBSD

# Compilation

As easy as `make`

# Running

Run `obsdfreqd` as root, quit with `Ctrl+C`.

# Usage

- `-h` show usage
- `-d downstepfrequency` sets the steps removed every cycle when decaying, default to 100
- `-i inertia` sets the number of cycles after which the frequency will decay
- `-m maxfrequency` sets the maximum frequency the CPU can reach in percent, 100% is default
- `-l minfrequency` sets the minimum frequency the CPU must be lowered to, 0% is default
- `-r threshold` sets the CPU usage in % that will trigger the frequency increase
- `-s stepfrequency` sets the percent of frequency added every cycle when increasing, 10% is default
- `-t timefreq` sets the milliseconds between each poll, 300 is the default

# Explanation

The current algorithm works this way:

If CPU usage > `threshold`, increase frequency by `stepfrequency` up to `maxfrequency` every `timefreq` milliseconds and keep this frequency at least `inertia` cycles.

If CPU usage <= `threshold`, reduce frequency by `downstepfrequency` down to `minfrequency` every `timefreq` milliseconds when `inertia` reached 0. `inertia` lose one point every cycle the CPU usage is below `threshold`.

# What is it doing?

**obsdfreqd** will change the perfpolicy sysctl to manual and will change the CPU frequency by polling every often (like 100ms) the CPU load and change the frequency accordingly, the perfpolicy is set to auto upon exit.

The end goal is to provide a feature rich CPU frequency scheduler for the following use case:

- battery saving while keeping responsiveness when needed (which apm -L doesn't do)
- reduce heat or electrical coil noise when on powerplug because the new default assuming mainboard and CPU can manage itself doesn't work well
- reduce power consumption for system on powerplug while staying performant enough
- provide setings for minimum and maximum frequency available in some automatic mode (people may have $reasons to use this)

# Relation to the OpenBSD project

This is mostly a playground project so I can experiment with CPU frequency scheduling, there is no goal to import it into the OpenBSD kernel ever, but maybe I can learn here and improve the kernel code later.
