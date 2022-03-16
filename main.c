#include <stdio.h>
#include <stdlib.h>
#include <err.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#include <time.h>
#include <sys/sched.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>


/* define the policy to auto or manual */
void set_policy(const char* policy) {
    int mib[2];
    size_t len;

    mib[0] = CTL_HW;
    mib[1] = HW_PERFPOLICY;
    len = sizeof(policy);
    if (sysctl(mib, 2, NULL, 0, policy, len) == -1)
        err(1, "sysctl");
}

/* restore policy auto upon exit */
void quit_gracefully() {
    set_policy("auto");
    exit(0);
}

int main() {

    int mib_perf[2];
    int mib_powerplug[2];
    int mib_load[2];
    long cpu[CPUSTATES], cpu_previous[CPUSTATES];
    int frequency;
    int value;
    int cpu_usage_total = 0, cpu_usage;
    size_t len, len_cpu;

    mib_perf[0] = CTL_HW;
    mib_perf[1] = HW_SETPERF;

    mib_powerplug[0] = CTL_HW;
    mib_powerplug[1] = HW_POWER;

    mib_load[0] = CTL_KERN;
    mib_load[1] = KERN_CPTIME;

    len = sizeof(value);
    len_cpu = sizeof(cpu);

    signal(SIGINT, quit_gracefully);
    set_policy("manual");

    // avoid weird reading for first delta
    if (sysctl(mib_load, 2, &cpu_previous, &len_cpu, NULL, 0) == -1)
        err(1, "sysctl");

    /* main loop */
    for(;;) {
        // get if using power plug or not
        if (sysctl(mib_powerplug, 2, &value, &len, NULL, 0) == -1)
            err(1, "sysctl");
        printf("power: %i |", value);

        // get current frequency
        if (sysctl(mib_perf, 2, &value, &len, NULL, 0) == -1)
            err(1, "sysctl");
        printf("perf: %3i |", frequency);

        // get where the CPU time is spent, last field is IDLE
        if (sysctl(mib_load, 2, &cpu, &len_cpu, NULL, 0) == -1)
            err(1, "sysctl");

        // calculate delta between old and last cpu readings
        cpu_usage = cpu[0]-cpu_previous[0] +
            cpu[1]-cpu_previous[1] +
            cpu[2]-cpu_previous[2] +
            cpu[3]-cpu_previous[3] +
            cpu[4]-cpu_previous[4];

        if(cpu_usage == 0)
            cpu_usage_total = 0;
        else
            cpu_usage_total = 100-((cpu[5]-cpu_previous[5])-cpu_usage);
        memcpy(cpu_previous, cpu, sizeof(cpu));
        printf("usage: %3i%% |", cpu_usage_total);

        // change frequency
        len = sizeof(frequency);
        if(cpu_usage_total > 30) {
            if(frequency < 100)
                frequency = frequency + 10;
            if (sysctl(mib_perf, 2, NULL, 0, &frequency, len) == -1)
                err(1, "sysctl");
        }
        else {
            frequency = 0;
            if (sysctl(mib_perf, 2, NULL, 0, &frequency, len) == -1)
                err(1, "sysctl");
        }

        printf("new freq: %3i |", frequency);

        printf("\n");
        usleep(1000*300);
    }

   return(0);
}
