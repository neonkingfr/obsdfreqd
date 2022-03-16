# obsdfreqd

Userland CPU frequency scheduling for OpenBSD

# Compilation

As easy as `make`

# Running

Run `obsdfreqd` as root, quit with `Ctrl+C`.

# What is it doing?

**obsdfreqd** will change the perfpolicy sysctl to manual and will change the CPU frequency by polling every often (like 100ms) the CPU load and change the frequency accordingly, the perfpolicy is set to auto upon exit.

The end goal is to provide a feature rich CPU frequency scheduler for the following use case:

- battery saving while keeping responsiveness when needed (which apm -L doesn't do)
- reduce heat or electrical coil noise when on powerplug because the new default assuming mainboard and CPU can manage itself doesn't work well
- reduce power consumption for system on powerplug while staying performant enough
