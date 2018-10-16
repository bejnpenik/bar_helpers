#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

char capacity_command[] = "/sys/class/power_supply/BAT0/capacity";
char status_command[] = "/sys/class/power_supply/BAT0/status";
char DATA[256] = {0};

typedef enum {
	CHARGING,
	DISCHARGING
} status_t;
int battery_level_new;
int battery_level_old;
status_t status_new;
status_t status_old;
int status_change(){
	if (status_new == status_old) return 0;
	status_old = status_new;
	return 1;
}
int level_change(){
	if (battery_level_new == battery_level_old) return 0;
	battery_level_old = battery_level_new;
	return 1;
	
}

FILE* bdopen(char const *cname, char leave_open){
	FILE *fin = fopen(cname, "r");
	setvbuf(fin, NULL, _IONBF, 0);
	fgets(DATA, 256, fin);
	if(leave_open==0){
		fclose(fin);
		return NULL;
	}
	else return fin;
}
int main (){
	FILE *fcap = bdopen(capacity_command, 1);
	battery_level_new = battery_level_old = atoi(DATA);
	FILE *fstat = bdopen(status_command, 1);
	if (strncmp(DATA, "Charging", 8)==0) status_new = status_old = CHARGING;
	else if (strncmp(DATA, "Discharging", 11)==0) status_new = status_old = DISCHARGING;
	if (status_new == CHARGING) printf("BAT;Charging: %d", battery_level_new);
	else if (status_new == DISCHARGING) printf("BAT;Discharging: %d", battery_level_new);
	fflush(stdout);
	while (1){
		fseek(fcap, 0, SEEK_SET);
		fgets(DATA, 256, fcap);
		battery_level_new = atoi(DATA);
		fseek(fstat, 0, SEEK_SET);
		fgets(DATA, 256, fstat);
		if (strncmp(DATA, "Charging", 8)==0) status_new = CHARGING;
		else if (strncmp(DATA, "Discharging", 11)==0) status_new = DISCHARGING;
		if (status_change() || level_change()){
			if (status_new == CHARGING) printf("BAT;Charging: %d", battery_level_new);
			else if (status_new == DISCHARGING) printf("BAT;Discharging: %d", battery_level_new);
			fflush(stdout);
		}
		sleep(1);
			
		
	}
	
}
int main_old(int argc, char const *argv[]) {
	
	FILE *fcap = NULL;
	FILE *fstat = NULL;
	fcap = popen(capacity_command, "r");
	if (fcap == NULL) return 1;
	if (fgets(DATA, 256, fcap) != NULL)
   		battery_level_new = battery_level_old = atoi(DATA);
	pclose(fcap);
	memset(DATA, 0, 256);
	fstat = popen(status_command, "r");
	if (fstat == NULL) return 1;
	if (fgets(DATA, 256, fstat) != NULL)
   		if (strncmp(DATA, "Charging", 8)==0) status_new = status_old = CHARGING;
		else if (strncmp(DATA, "Discharging", 11)==0) status_new = status_old = DISCHARGING;
	pclose(fstat);
	while (1){
		if (status_change() || level_change())
			if (status_new == CHARGING) printf("Charging: %d\n", battery_level_new);
			else if (status_new == DISCHARGING) printf("Discharging: %d\n", battery_level_new);
		sleep(1);
	}
	
	return 0;
}

