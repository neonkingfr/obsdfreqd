#include <stdio.h>
#include <stdlib.h>
#include <err.h>
#include <math.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#include <time.h>
#include <sys/sched.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>


/* define the policy to auto or manual */
void set_policy(char* policy) {
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

void usage() {
    printf("obsdfreqd [-l min_freq]  [-m max_freq] [-s step]\n");
}

int main(int argc, char *argv[]) {

    int opt;
    int mib_perf[2];
    int mib_powerplug[2];
    int mib_load[2];
    long cpu[CPUSTATES], cpu_previous[CPUSTATES];
    int frequency;
    int value, current_frequency;
    int cpu_usage_total = 0, cpu_usage;
    size_t len, len_cpu;

    unveil("/", "r");
    unveil(NULL, NULL);

    int min = 0;
    int max = 100;
    int step = 10;

    while((opt = getopt(argc, argv, "hl:m:s:")) != -1) {
        switch(opt) {
        case 'l':
            min = atoi(optarg);
            if(min > 100 || min < 0)
                err(1, "minimum frequency must be between 0 and 100");
            break;
        case 'm':
            max = atoi(optarg);
            if(max > 100 || max < 0)
                err(1, "maximum frequency must be between 0 and 100");
            break;
        case 's':
            step = atoi(optarg);
            if(step > 100 || step <= 0)
                err(1, "step must be positive and up to 100");
            break;
        case 'h':
        default:
           usage();
           return 1;
        }
    }

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
    usleep(1000*500);

    /* main loop */
    for(;;) {
        // get if using power plug or not
        if (sysctl(mib_powerplug, 2, &value, &len, NULL, 0) == -1)
            err(1, "sysctl");
        printf("power: %i |", value);

        // get current frequency
        if (sysctl(mib_perf, 2, &current_frequency, &len, NULL, 0) == -1)
            err(1, "sysctl");
        printf("perf: %3i |", current_frequency);

        // get where the CPU time is spent, last field is IDLE
        if (sysctl(mib_load, 2, &cpu, &len_cpu, NULL, 0) == -1)
            err(1, "sysctl");

        // calculate delta between old and last cpu readings
        cpu_usage = cpu[0]-cpu_previous[0] +
            cpu[1]-cpu_previous[1] +
            cpu[2]-cpu_previous[2] +
            cpu[3]-cpu_previous[3] +
            cpu[4]-cpu_previous[4] +
            cpu[5]-cpu_previous[5];

        // debug
        //printf("\nDEBUG: User: %3i\tNice: %3i\t Sys: %3i\tSpin: %3i\t Intr: %3i\tIdle: %3i\n",
        //       cpu[0]-cpu_previous[0],
        //       cpu[1]-cpu_previous[1],
        //       cpu[2]-cpu_previous[2],
        //       cpu[3]-cpu_previous[3],
        //       cpu[4]-cpu_previous[4],
        //       cpu[5]-cpu_previous[5]);
        //printf("cpu usage = %i et idle = %i\n", cpu_usage, cpu[5] - cpu_previous[5]);

        cpu_usage_total = 100-round(100*(cpu[5]-cpu_previous[5])/cpu_usage);
        memcpy(cpu_previous, cpu, sizeof(cpu));
        printf("usage: %3i%% |", cpu_usage_total);

        // change frequency
        len = sizeof(frequency);

        // small brain condition to increase CPU
        if(cpu_usage_total > 30) {

            // increase frequency by step if under max
            if(frequency < max)
                frequency = frequency + step;

            // don't try to set frequency more than 100%
            if( frequency > 100 )
                frequency = 100;

            if (sysctl(mib_perf, 2, NULL, 0, &frequency, len) == -1)
                err(1, "sysctl");
        }
        else {
            frequency = min;
            if (sysctl(mib_perf, 2, NULL, 0, &frequency, len) == -1)
                err(1, "sysctl");
        }

        printf("new freq: %3i |", frequency);

        printf("\n");
        usleep(1000*300);
    }

   return(0);
}
