/*
 * (c) Copyright 2007 Federico Tomassini aka Efphe <efphe@freaknet.org>
 *
 * Changed a bit by AlpT (@freaknet.org)
 *
 * This source code is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as published 
 * by the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 *
 * This source code is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * Please refer to the GNU Public License for more details.
 *
 * You should have received a copy of the GNU Public License along with
 * this source code; if not, write to:
 * Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/* 
 * CONFIGURE HERE 
 */
#define FD_IFACE "eth0"
/**/


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#define FREQUENCE 256 /*ms*/
#define MAXBUF 256
void init_net_stats(void);

char *ratings[] = {"B/s", "K/s", "M/s", "G/s"};
char *bytemegagiga[] = {"B", "K", "M", "G"};
char tbufd[12];
char tbufu[12];
char tbufm[12];

typedef struct net_stasts
{
    unsigned long down;
    unsigned long  up;
    unsigned long nup;
    unsigned long ndown;
} net_stats;

static net_stats _stats;

int get_memusage();
int get_cpu_temperature();
int update_stats(unsigned long *up, unsigned long *down);

int getload(int cpunum, int shownice) {
	static int lastloadstuff = 0, lastidle = 0, lastnice = 0;
	int loadline = 0, loadstuff = 0, nice = 0, idle = 0;
	FILE *stat = fopen("/proc/stat", "r");
	char temp[128], *p;

	if (!stat) return 100;

	while (fgets(temp, 128, stat)) {
		if ((!strncmp(temp, "cpu", 3) && (temp[3] == ' ' && cpunum < 0)) || (temp[3] != ' ' && atol(&temp[3]) == cpunum)) {
			p = strtok(temp, " \t\n");
			loadstuff = atol(p = strtok(NULL, " \t\n"));
			nice = atol(p = strtok(NULL, " \t\n"));
			loadstuff += atol(p = strtok(NULL, " \t\n"));
			idle = atol(p = strtok(NULL, " \t\n"));
			break;
		}
	}

	fclose(stat);

	if (!lastloadstuff && !lastidle && !lastnice) {
		lastloadstuff = loadstuff; lastidle = idle; lastnice = nice;
		return 0;
	}

	if (shownice) {
		loadline = (loadstuff-lastloadstuff)+(idle-lastidle);
	} else {
		loadline = (loadstuff-lastloadstuff)+(nice-lastnice)+(idle-lastidle);
	}
	if (loadline)
		loadline = ((loadstuff-lastloadstuff)*100)/loadline;
	else loadline = 100;

	lastloadstuff = loadstuff; lastidle = idle; lastnice = nice;

	return loadline;
}


void charize(unsigned long rate, char *where, char *ratings[])
{
    char **p;
    float trate= (float)rate;

    p=ratings;
    while (trate > 1024) {
        p++;
        trate/= 1024;
    }
    if (trate > 999) {
        trate /= 1024;
        if ( p !=ratings) p--;
    }
    snprintf(where, 20, "%3d %s", (int)trate, *p);
    return;
}

int main(int argc, char **argv)
{
    int a,b,res, ct;
    struct tm *t;
    time_t tt;
    char sstime[20];
    unsigned long up, down;

    init_net_stats();

    while (1) {
        tt= time(0);
        t= localtime(&tt);
        strftime(sstime, 20, "%a %b %e %T", (const struct tm *)t);

        a=get_memusage();
        b=getload(-1, 1);
	ct=get_cpu_temperature();
        charize(a, tbufm, bytemegagiga);
        res=update_stats(&up, &down);

        printf("Cpu %3d%%,%2d C,  Mem %s", b, ct, tbufm);
	if(res != -1)
        	printf(",  tx:%s rx:%s",  tbufu, tbufd);
	printf(",  %s\n", sstime);

        /* Normalize */
        up/= FREQUENCE;
        down/= FREQUENCE;

        charize(up, tbufu, ratings);
        charize(down, tbufd, ratings);

        fflush(stdout);
        usleep(FREQUENCE * 1000);
    }
    return 0;
}

int get_memusage()
{
    FILE *meminfo;
    int total, free, buffers, cached;

    if (!(meminfo=fopen("/proc/meminfo", "r")))
        return -1;
    fscanf(meminfo, "%*s %d %*s\n" // memtotal
		    "%*s %d %*s\n"     // memfree
		    "%*s %d %*s\n"	   // buffers
		    "%*s %d %*s\n"	   // cached
		    , &total, &free, &buffers, &cached);
    fclose(meminfo);
    return (total-(free+buffers+cached))*1024;
}
    
int linux_proc_get(unsigned long *up, unsigned long *down)
{
  FILE* fp;
  char temp[MAXBUF];
  char* p;
  int active = -1;

  /* read statistics from network device's list */
  fp = fopen("/proc/net/dev", "r");
  if(!fp) return -1;

  fgets(temp, MAXBUF, fp);
  fgets(temp, MAXBUF, fp);

  while(fgets(temp, MAXBUF, fp))
    if(strstr(temp, FD_IFACE))
    {
      p = strchr(temp, ':');
      ++p;
      sscanf(p, "%lu %*s %*s %*s %*s %*s %*s %*s %lu", down, up);
      active = 0;
      break;
    }
  fclose(fp);

  return active;
}

int get_cpu_temperature() {
  FILE* fp;
  int t;

  fp = fopen("/proc/acpi/thermal_zone/ATF0/temperature", "r");
  if(!fp) return -1;
  fscanf(fp, "%*s %d", &t);
  fclose(fp);

  return t;
}

void init_net_stats(void)
{
    unsigned long up, down;
    linux_proc_get(&up, &down);
    _stats.down= down;
    _stats.up= up;
    _stats.nup= 0;
    _stats.ndown= 0;

    return;
}

int update_stats(unsigned long *up, unsigned long *down)
{
    int res;

    res= linux_proc_get(up, down);
    if (res) 
        return -1;

    _stats.nup= *up - _stats.up;
    _stats.ndown= *down - _stats.down;
    _stats.up= *up;
    _stats.down= *down;
    *up= _stats.nup;
    *down= _stats.ndown;
    return 0;
}
