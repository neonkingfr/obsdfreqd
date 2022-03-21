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

int min,	batt_min,	wall_min;
int max,	batt_max,	wall_max;
int threshold,	batt_threshold,	wall_threshold;
int down_step,	batt_down_step,	wall_down_step;
int inertia,	batt_inertia,	wall_inertia;
int step,	batt_step,	wall_step;
int timefreq,	batt_timefreq,	wall_timefreq;
int temp_max,   batt_tmax,      wall_tmax;
int verbose = 0;

float get_temp(void);
void set_policy(const char*);
void quit_gracefully(int signum);
void usage(void);
void switch_wall(void);
void switch_batt(void);
void assign_values_from_param(char*, int*, int*);

float get_temp() {
    FILE *fp;
    char path[1035];
    char *eptr;

    fp = popen("/usr/sbin/sysctl -n hw.sensors.cpu0", "r");
    if (fp == NULL)
        err(1, "popen failed");
    fgets(path, sizeof(path), fp);

    char *token = strtok(path, " ");
    pclose(fp);
    return(strtof(token, &eptr));
}

/* define the policy to auto or manual */
void set_policy(const char* policy) {
    int mib[2] = { CTL_HW, HW_PERFPOLICY };

    if (sysctl(mib, 2, NULL, 0, (void *)policy, strlen(policy) + 1) == -1)
        err(1, "sysctl: setting policy");
}

/* restore policy auto upon exit */
void quit_gracefully(int signum) {
	printf("Caught signal %d, set auto policy\n", signum);
    set_policy("auto");
    exit(0);
}

void usage(void) {
    printf("obsdfreqd [-h] [-v] [-i cycles] [-l min_freq] [-m max_freq] [-d percent_down_freq_step] [-r threshold] [-s percent_freq_step] [-t milliseconds]\n");
}

/* switch to wall profile */
void switch_wall() {
    min = wall_min;
    max = wall_max;
    threshold = wall_threshold;
    down_step = wall_down_step;
    inertia = wall_inertia;
    step = wall_step;
    timefreq = wall_timefreq;
    temp_max = wall_tmax;
}

/* switch to battery profile */
void switch_batt() {
    min = batt_min;
    max = batt_max;
    threshold = batt_threshold;
    down_step = batt_down_step;
    inertia = batt_inertia;
    step = batt_step;
    timefreq = batt_timefreq;
    temp_max = batt_tmax;
}

/* assign values to variable if comma separated
 * if not, assign value to two variables
 */
void assign_values_from_param(char* parameter, int* charging, int* battery) {
    int count = 0;
    char *token = strtok(parameter, ",");

    while (token != NULL) {
        if(count == 0)
            *charging = atoi(token);
        if(count == 1)
            *battery = atoi(token);
        token = strtok(NULL, ",");
        count++;
        if(count > 1)
            break;
    }

    if(count == 0) {
        *charging = atoi(parameter);
        *battery = *charging;
    }

    if(count == 1)
        *battery = *charging;
}

int main(int argc, char *argv[]) {

    int opt;
    int mib_perf[2];
    int mib_powerplug[2];
    int mib_load[2];
    long cpu[CPUSTATES], cpu_previous[CPUSTATES];
    int frequency = 0;
    int current_mode;
    int value, current_frequency, inertia_timer = 0;
    int cpu_usage_percent = 0, cpu_usage;
    float temp;
    size_t len, len_cpu;

    // battery defaults
    min =	batt_min=	0;
    max =	batt_max=	100;
    threshold =	batt_threshold=	30;
    down_step =	batt_down_step=	100;
    inertia =	batt_inertia=	5;
    step =	batt_step=	100;
    timefreq =	batt_timefreq=	100;
    temp_max =	batt_tmax=	0;

    // wall defaults
    wall_min=		0;
    wall_max=		100;
    wall_threshold=	30;
    wall_down_step=	30;
    wall_inertia=	5;
    wall_step=		100;
    wall_timefreq=	100;
    wall_tmax=		0;

    //if (unveil("/var/empty", "r") == -1)
    //    err(1, "unveil failed");
    //unveil(NULL, NULL);

    while((opt = getopt(argc, argv, "d:hi:l:m:r:s:t:T:v")) != -1) {
        switch(opt) {
        case 'd':
            assign_values_from_param(optarg, &wall_down_step, &batt_down_step);
            if(down_step > 100 || down_step <= 0)
                err(1, "decay step must be positive and up to 100");
            break;
        case 'i':
            assign_values_from_param(optarg, &wall_inertia, &batt_inertia);
            if(inertia < 0)
                err(1, "inertia must be positive");
            break;
        case 'l':
            assign_values_from_param(optarg, &wall_min, &batt_min);
            if(min > 100 || min < 0)
                err(1, "minimum frequency must be between 0 and 100");
            break;
        case 'm':
            assign_values_from_param(optarg, &wall_max, &batt_max);
            if(max > 100 || max < 0)
                err(1, "maximum frequency must be between 0 and 100");
            break;
        case 'v':
            verbose = 1;
            break;
        case 'r':
            assign_values_from_param(optarg, &wall_threshold, &batt_threshold);
             if(threshold < 0)
                 err(1, "CPU use threshold must be positive");
             break;
        case 's':
            assign_values_from_param(optarg, &wall_step, &batt_step);
            if(step > 100 || step <= 0)
                err(1, "step must be positive and up to 100");
            break;
        case 't':
            assign_values_from_param(optarg, &wall_timefreq, &batt_timefreq);
            if(wall_timefreq <= 0 || batt_timefreq <= 0)
                err(1, "time frequency must be positive");
            break;
        case 'T':
            assign_values_from_param(optarg, &wall_tmax, &batt_tmax);
            if(wall_tmax <= 0 || batt_tmax <= 0)
                err(1, "temperature must be positive");
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

    signal(SIGINT,  quit_gracefully);
    signal(SIGTERM, quit_gracefully);
    set_policy("manual");

    if (verbose) {
        if(temp_max > 0) {
            printf("mode;Temperature;maximum_frequency;current_frequency;cpu usage;inertia;new frequency\n");
        } else {
            printf("mode;current_frequency;cpu usage;inertia;new frequency\n");
        }
    }

    /* avoid weird reading for first delta */
    if (sysctl(mib_load, 2, &cpu_previous, &len_cpu, NULL, 0) == -1)
        err(1, "sysctl");
    usleep(1000*500);

    /* main loop */
    for(;;) {
        /* get if using power plug or not */
        if (sysctl(mib_powerplug, 2, &value, &len, NULL, 0) == -1)
            err(1, "sysctl");

        if(verbose) printf("%i;", value);

        if(current_mode != value) {
            current_mode = value;
            if(value == 0)
                switch_batt();
            else
                switch_wall();
        }

        // manage temperature
        if(temp_max > 0) {
            temp = get_temp();
            if(temp > temp_max) {
                if(max > min)
                    max--;
            } else {
                if(max < 100)
                    max++;
            }
            if(verbose) printf("%.0f;%i;", temp, max);
        }

        /* get current frequency */
        if (sysctl(mib_perf, 2, &current_frequency, &len, NULL, 0) == -1)
            err(1, "sysctl");
        if(verbose) printf("%i;", current_frequency);

        /* get where the CPU time is spent, last field is IDLE */
        if (sysctl(mib_load, 2, &cpu, &len_cpu, NULL, 0) == -1)
            err(1, "sysctl");

        /* calculate delta between old and last cpu readings */
        cpu_usage = cpu[0]-cpu_previous[0] +
            cpu[1]-cpu_previous[1] +
            cpu[2]-cpu_previous[2] +
            cpu[3]-cpu_previous[3] +
            cpu[4]-cpu_previous[4] +
            cpu[5]-cpu_previous[5];

        cpu_usage_percent = 100-round(100*(cpu[5]-cpu_previous[5])/cpu_usage);
        memcpy(cpu_previous, cpu, sizeof(cpu));
        if(verbose) printf("%i;", cpu_usage_percent);

        /* change frequency */
        len = sizeof(frequency);

        /* small brain condition to increase CPU */
        if(cpu_usage_percent > threshold) {

            /* increase frequency by step if under max */
            if(frequency+step < max)
                frequency = frequency + step;
            else
                frequency = max;

            /* don't try to set frequency more than 100% */
            if( frequency > 100 )
                frequency = 100;

	    if(inertia_timer < inertia)
                inertia_timer++;

            if (sysctl(mib_perf, 2, NULL, 0, &frequency, len) == -1)
                err(1, "sysctl");

        } else {

            if(inertia_timer == 0) {
                /* keep frequency more than min */
                if(frequency-down_step < min)
                    frequency = min;
                else
                    frequency = frequency - down_step;

                /* don't try to set frequency below 0% */
                if (frequency < 0 )
                    frequency = 0;

                if (sysctl(mib_perf, 2, NULL, 0, &frequency, len) == -1)
                    err(1, "sysctl");
            } else {
                inertia_timer--;
            }
        }

        if(verbose) printf("%i;%i\n", inertia_timer, frequency);

        usleep(1000*timefreq);
    }

   return(0);
}
