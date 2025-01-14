/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell 
 * or otherwise commercially exploit the source or things you created based on the 
 * source.
 *
*/




#include <cstdarg>
#include "network/multi_log.h"
#include "parse/generic_log.h"
#include "cfile/cfile.h"
#include "parse/parselo.h"
#include "globalincs/version.h"
#include "network/multi_options.h"
#include "network/multi_fstracker.h"
#include "network/multi.h"



// ----------------------------------------------------------------------------------------------------
// MULTI LOGFILE DEFINES/VARS
//

// how often we'll write an update to the logfile (in seconds)
#define MULTI_LOGFILE_UPDATE_TIME			2520			// every 42 minutes

// time when the logfile was opened
int Multi_log_open_systime = -1;

// time when we last updated the logfile
int Multi_log_update_systime = -1;

// ----------------------------------------------------------------------------------------------------
// MULTI LOGFILE FUNCTIONS
//

// write the standard header to the logfile
void multi_log_write_header()
{
	char str[1024];
	time_t timer;	

	// header message
	timer = time(NULL);	
	strftime(str, 1024, "FreeSpace Multi Log - Opened %a, %b %d, %Y  at %I:%M%p\n----\n----\n----\n\n", localtime(&timer));
	log_string(LOGFILE_MULTI_LOG, str, 0);	
}

// write the standard shutdown trailer
void multi_log_write_trailer()
{
	char str[1024];
	time_t timer;		

	// header message
	timer = time(NULL);
	strftime(str, 1024, "\n\n----\n----\n----\nFreeSpace Multi Log - Closing on %a, %b %d, %Y  at %I:%M%p", localtime(&timer));
	log_string(LOGFILE_MULTI_LOG, str, 0);	
}

// write out some info about stuff
void multi_log_write_update()
{
	int diff = (int)difftime(time(NULL), Multi_log_open_systime);
	int hours, mins, seconds;

	// figure out some time values
	hours = diff / 3600;
	mins = (diff - (hours * 3600)) / 60;
	seconds = (diff - (hours * 3600) - (mins * 60));

	// print it out
	ml_printf("Server has been active for %d hours, %d minutes, and %d seconds", hours, mins, seconds);
}

// write out some info helpful for debugging
static void multi_log_write_info()
{
	extern bool Multi_cfg_missing;

	if (Multi_cfg_missing) {
		ml_string("**  multi.cfg is missing!  **");
	}

	ml_printf("FreeSpace 2 Open version: %s", FS_VERSION_FULL);
	ml_printf("Multi version: %d", MULTI_FS_SERVER_VERSION);

	extern void cmdline_print_cmdline_multi();
	cmdline_print_cmdline_multi();

	if (Is_standalone) {
		ml_printf("PXO: %s", Multi_options_g.pxo ? "Enabled" : "Disabled");

		if (Multi_options_g.pxo) {
			ml_printf("PXO Channel: %s", Multi_fs_tracker_channel);
		}
	}
}

// initialize the multi logfile
void multi_log_init()
{
	if (logfile_init(LOGFILE_MULTI_LOG)) {
		multi_log_write_header();
		multi_log_write_info();

		// initialize our timer info
		Multi_log_open_systime = (int) time(NULL);
		Multi_log_update_systime = Multi_log_open_systime;
	} 
}

// close down the multi logfile
void multi_log_close()
{
	multi_log_write_trailer();
	logfile_close(LOGFILE_MULTI_LOG);
}

// give some processing time to the logfile system so it can check up on stuff
void multi_log_process()
{
	// check to see if we've been active a long enough time, and 
	if(time(NULL) - Multi_log_update_systime > MULTI_LOGFILE_UPDATE_TIME){
		// write the update
		multi_log_write_update();

		Multi_log_update_systime = (int) time(NULL);
	}
}

// printf function itself called by the ml_printf macro
void ml_printf(const char *format, ...)
{
	SCP_string temp;
	va_list args;

	if (format == NULL) {
		return;
	}
	
	// format the text
	va_start(args, format);
	vsprintf(temp, format, args);
	va_end(args);

	// log the string including the time
	log_string(LOGFILE_MULTI_LOG, temp.c_str(), 1);
}

// string print function
void ml_string(const char *string, int add_time)
{
	SCP_string tmp;
	char time_str[128];
	time_t timer;	

	// if the passed string is NULL, do nothing
	if(string == NULL){
		return;
	}

	// maybe add the time
	if(add_time){
		timer = time(NULL);

		strftime(time_str, 128, "%m/%d %H:%M:%S~   ", localtime(&timer));
		tmp = time_str;
		tmp += string;
	} else{
		tmp = string;
	}
	// don't need to add terminating \n since log_string() will do it

	// now print it to the logfile if necessary	
	log_string(LOGFILE_MULTI_LOG, tmp.c_str(), 0);

	// add to standalone UI too
	extern int Is_standalone;
	extern void std_debug_multilog_add_line(const char *str);
	if (Is_standalone) {
		std_debug_multilog_add_line(tmp.c_str());
	}

#if defined(MULTI_LOGFILE_ECHO_TO_DEBUG)
	// nprintf(("Network","%s\n",tmp));
	mprintf(("ML %s", tmp.c_str()));
#endif
}
