/* 
 * Author: Stiletto <blasux@blasux.ru> 2011
 *
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/wtfpl/COPYING for more details. */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "config.h"


struct t_service {
    char name[32];
};

struct t_step {
    struct t_service *service;
    int count;
};

struct t_step steps[128];
int scount = 0;
int stotal = 0;
int sdone = 0;

void panic(char *msg) {
    printf("%s: Error %d: %s\n",msg,errno,strerror(errno));
    exit(errno);
}

void startstop_step(struct t_step step,char *action) {
    for (int i=0;i<step.count;i++) printf(" %s",step.service[i].name);
    printf("\n");
    for (int i=0;i<step.count;i++) {
        if(!fork()) {
            char snamebuf[1024];
            int new_stdout, new_stdin;
            
            strncpy(snamebuf,SERVICEDIR,1024);
            strcat(snamebuf,step.service[i].name);
            
            new_stdin = open(TEXT_TTY,O_RDONLY);
            new_stdout = open(TEXT_TTY,O_WRONLY);
            if ((new_stdin==-1)||(new_stdout==-1)) {
        	printf(CERROR"Couldn't open tty %s ",TEXT_TTY);
        	panic(snamebuf);
	    }
	    
	    if ((dup2(new_stdin,0)==-1)||(dup2(new_stdout,1)==-1)||(dup2(new_stdout,2)==-1)) {
        	printf(CERROR"Couldn't dup2() ");
        	panic(snamebuf);
	    }
            
            execl(snamebuf,snamebuf,action,NULL);
            printf(CERROR"Couldn't exec ");
            panic(snamebuf);
        }
    }

}

void print_progress(int done, int total) {
    printf(" [");
    for (int i=0;i<total;i++) {
	if (i<=done) 
	    printf("#");
	else
	    printf("-");
    }
    printf("] %d/%d",done,total);
    fflush(stdout);
}

void wait_step(int count,int done,int total) {
    for (int i=0;i<count;i++) {
        int status;
        waitpid(-1,&status,0);
        printf("\r");
        print_progress(done+i+1,total);
    }
}

int main(int argc, char *argv[]) {
    if (argc<3) {
        printf("Usage: %s <configfile> <start|stop>\n",argv[0]);
        exit(1);
    }
    FILE *config = fopen(argv[1],"r");
    if (!config) panic("fopen(CONFIG,\"r\")");
    char buf[1024];
    scount = 0;
    int len;
    printf(CNORMAL"Loading configuration...");
    while (fgets(buf,1024,config)) {
        buf[1023]='\0';
        char *comment = index(buf,'#');
        if (comment) *comment='\0';
        len=strlen(buf);
        if ((buf[len-1]=='\n'))
            buf[len-1]='\0';
        
        steps[scount].service = malloc(sizeof(struct t_service)*128);
        int count = 0;
        char *t = strtok(buf," ");
        while (t) {
            strncpy(steps[scount].service[count].name,t,32);
            steps[scount].service[count].name[31]='\0';
            count++;
            t = strtok(NULL," ");
        }
        if (count) {
            steps[scount].count = count;
            stotal += count;
            scount++;
        } else {
            free(steps[scount].service);
        }
    }
    printf("done.\n");
    if (!strcmp(argv[2],"start")) {
        printf(CNORMAL"Starting services...\n");
        for(int i=0;i<scount;i++) {
            printf(CNORMAL"Step %d/%d:",i+1,scount);
            startstop_step(steps[i],"start");
            wait_step(steps[i].count,sdone,stotal);
            printf("\n");
            sdone += steps[i].count;
        }
    } else if (!strcmp(argv[2],"stop")) {
        printf(CNORMAL"Stopping services...\n");
        for(int i=scount-1;i>=0;i--) {
            printf(CNORMAL"Step %d/%d:",scount-i,scount);
            startstop_step(steps[i],"stop");
            wait_step(steps[i].count,sdone,stotal);
            printf("\n");
            sdone += steps[i].count;
        }
    } else {
        printf(CNORMAL"Doing nothing. Configuration is ok.\n");
    }
    printf(CWIN"Done.\n");
    
    if (errno) panic("fgets");
    fclose(config);
}
