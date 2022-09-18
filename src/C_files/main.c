#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "C_files/GPhoto2D.h"
#include "C_files/GPhoto2DCmd.h"
// #define MAX_COMMAND_LEN 256

// int pipeFD;
// char readbuf[2 * MAX_COMMAND_LEN];

// #define FIFO_MAX 10
// // extern char readbuf[FIFO_MAX][2 * MAX_COMMAND_LEN];
// extern int fd[FIFO_MAX], readi[FIFO_MAX];

/* Inter-process Communication with FIFOs:
Note: For illustration purposes, we don't mind what the data is, so we don't bother to initialize buffer.

By: Subhasish Ghosh
Date: August 17th 2001
Place: Calcutta, WB, India
E-mail: subhasish_ghosh@linuxmail.org
*/

#include <fcntl.h>
#include <gphoto2/gphoto2-camera.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define BUFFER_SIZE 512
#define TEN_MEG (1024 * 1024 * 10)

// struct pipe_file
// {
// 	const char* file;
// 	const int open_mode;
// };

// void printLog(char *msg, ...)
// {
// 	char *timestamp;
// 	va_list args;
// 	va_start(args, msg);
// 	int nofile = 0;
// 	FILE *fp;
// 	vfprintf(stdout, msg, args);

// 	// if (cfg_stru[c_log_file] != 0 && cfg_val[c_log_size] != 0) {
// 	// 	nofile = (access(cfg_stru[c_log_file], F_OK ) == -1 );
// 	// 	fp = fopen(cfg_stru[c_log_file], "a");
// 	// } else {
// 	// 	fp = stdout;
// 	// }
// 	// if (fp != NULL) {
// 	// 	clock_gettime(CLOCK_REALTIME, &currTime);
// 	// 	localTime = localtime (&(currTime.tv_sec));
// 	// 	makeName(&timestamp, "{%Y/%M/%D %h:%m:%s} ");
// 	// 	fprintf(fp, "%s",timestamp);
// 	// 	vfprintf(fp, msg, args);
// 	// 	if (cfg_stru[c_log_file] != 0) {
// 	// 		fclose(fp);
// 	// 		if (nofile) chmod(cfg_stru[c_log_file], 0777);
// 	// 	}
// 	// 	free(timestamp);
// 	// }
// 	va_end(args);
// }

char* commandBuffer[BUFFER_SIZE];

int getFifoFD(const char *file, int open_mode)
{
	int res = 0;
	if (access(file, F_OK) == -1)
	{
		res = mkfifo(file, 0777);
		if (res != 0)
		{
			printLog("Could not create fifo %s\n", file);
			exit(EXIT_FAILURE);
		}
	}
	int pipe_fd = open(file, open_mode);
	if (pipe_fd == -1)
	{
		printLog("Could not open fifo %s in %i\n", file, open_mode);
		exit(EXIT_FAILURE);
	}
	return pipe_fd;
}

enum GPhotoDCmds
{
	capture_image_cmd
};

void readPipe(const int pipe)
{
	int length;
	length = read(pipe, commandBuffer, BUFFER_SIZE);

	if(length > 0)
	{
		gphoto2_process_commands((uint8_t*)commandBuffer, length);

		// uint8_t cmd = (uint8_t)commandBuffer[0];
		// if(cmd == capture_image_cmd)
		// {
		// 	//DO capture photo
		// 	capturePhoto();
		// }
	}
}

// void checkPipe(int pipe) {
//    char *lf;
//    int length, hPipe;
//    hPipe = fd[pipe];
//    if(hPipe >= 0) {
// 	  length = read(hPipe, readbuf[pipe] + readi[pipe], MAX_COMMAND_LEN - 2);
// 	  if (length > 0) readi[pipe] +=length;

// 	  if(readi[pipe] != 0) {
// 		lf = strchr(readbuf[pipe], 10);
// 		if (lf != NULL) {
// 		  *lf = 0;
// 		  length = lf - readbuf[pipe];
// 		  readi[pipe] -= length + 1;
// 		  process_cmd(readbuf[pipe], length);
// 		  length = readbuf[pipe] + 2 * MAX_COMMAND_LEN - 1 - lf;
// 		  strncpy(readbuf[pipe], lf + 1, length);
// 		} else {
// 		  if ((length == 0) && (cfg_val[c_enforce_lf] == 0)) {
// 			process_cmd(readbuf[pipe], readi[pipe]);
// 			readi[pipe] = 0;			
// 		  }
// 		}
// 	  }
//    }
// 


int running = 1;

int main()
{
	printLog("Starting GPhoto2 Deamon");


	//TODO detect cameras

	// struct pipe_file input = {"/tmp/gphoto2In", O_RDONLY};
	// struct pipe_file output = {"/tmp/gphoto2Out", O_WRONLY};

	// int input_fd = getFifoFD("/home/threedean/Documents/Temp/gphoto2In", O_WRONLY);
	int input_fd = getFifoFD("/home/threedean/Documents/Temp/gphoto2In", O_RDONLY | O_NONBLOCK);
	int output_fd = -1;
	
	gphoto2_initialize();
	int cameraIndex;



	getFifoFD("/home/threedean/Documents/Temp/gphoto2Out", O_WRONLY);

	int bytes_sent = 0;
	char buffer[BUFFER_SIZE + 1];

	while (running == 1)
	{
		readPipe(input_fd);



	}
	// while (bytes_sent < TEN_MEG)
	// {
	// 	res = write(pipe_fd, buffer, BUFFER_SIZE);
	// 	if (res == -1)
	// 	{
	// 		fprintf(stderr, "Write error on page\n");
	// 		exit(EXIT_FAILURE);
	// 	}
	// 	bytes_sent += res;
	// }

	printLog("Shutting GPhoto2 Deamon");

	close(input_fd);
	close(output_fd);
}
// else
// {
// 	exit(EXIT_FAILURE);
// }

// printf("Process %d finished\n", getpid());
// exit(EXIT_SUCCESS);
// }

// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <unistd.h>

// int main()
// {

// 	int data_processed;
// 	int file_pipes[2];
// 	const char some_data[] = "123";
// 	char buffer[BUFSIZ + 1];
// 	pid_t fork_result;

// 	memset(buffer, '\0', sizeof(buffer));

// 	if (pipe(file_pipes) == 0)
// 	{
// 		fork_result = fork();
// 		if (fork_result == -1)
// 		{
// 			fprintf(stderr, "Fork Failure");
// 			exit(EXIT_FAILURE);
// 		}

// 		if (fork_result == 0)
// 		{
// 			data_processed = read(file_pipes[0], buffer, BUFSIZ);
// 			printf("Read %d bytes: %s\n", data_processed, buffer);
// 			exit(EXIT_SUCCESS);
// 		}
// 		else
// 		{
// 			data_processed = write(file_pipes[1], some_data, strlen(some_data));
// 			printf("Wrote %d bytes\n", data_processed);
// 		}
// 	}
// 	exit(EXIT_SUCCESS);
// }

// unsigned char timelapse = 0, running = 1, autostart = 1, idle = 0, a_error = 0, v_capturing = 0, i_capturing = 0, v_boxing = 0;

// int main(int argc, char *argv[])
// {
// 	int i, length;
// 	int watchdog = 0, watchdog_errors = 0;
// 	int onesec_check = 0;
// 	time_t last_pv_time = 0, pv_time;
// 	char fdName[FIFO_MAX][128];

// 	// Clear out anything in FIFO(s) first
// 	do
// 	{
// 		pipeFD = open(fdName, O_RDONLY | O_NONBLOCK);
// 		if (pipeFD >= 0)
// 		{
// 			fcntl(pipeFD, F_SETFL, 0);
// 			// Read buffer
// 			length = read(pipeFD, readbuf[0], 60);
// 			close(pipeFD);
// 		}
// 		else
// 		{
// 			if (i == 0)
// 				error("Could not open main PIPE", 1);
// 		}
// 	} while (pipeFD >= 0 && length != 0);

// 	// Main forever loop
// 	for (i = 0; i < FIFO_MAX; i++)
// 	{
// 		fd[i] = open(fdName[i], O_RDONLY | O_NONBLOCK);
// 		if (fd[i] >= 0)
// 		{
// 			printLog("Opening FIFO %i %s %i\n", i, fdName[i], fd[i]);
// 			fcntl(fd[i], F_SETFL, 0);
// 		}
// 	}

// 	updateStatus();
// 	// Send restart signal to scheduler
// 	send_schedulecmd("9");

// 	printLog("Starting command loop\n");
// 	if (cfg_val[c_fifo_interval] < 100000)
// 		cfg_val[c_fifo_interval] = 100000;

// 	while (running == 1)
// 	{
// 		for (i = 0; i < FIFO_MAX; i++)
// 		{
// 			checkPipe(i);
// 		}

// 		if (timelapse)
// 		{
// 			tl_cnt++;
// 			if (tl_cnt >= cfg_val[c_tl_interval])
// 			{
// 				if (i_capturing == 0)
// 				{
// 					capt_img();
// 					tl_cnt = 0;
// 				}
// 			}
// 		}
// 		// check to see if image preview changing
// 		if (!idle && cfg_val[c_watchdog_interval] > 0)
// 		{
// 			if (watchdog++ > cfg_val[c_watchdog_interval])
// 			{
// 				watchdog = 0;
// 				pv_time = get_mtime(cfg_stru[c_preview_path]);
// 				if (pv_time == 0)
// 				{
// 					watchdog_errors++;
// 				}
// 				else
// 				{
// 					if (pv_time > last_pv_time)
// 					{
// 						watchdog_errors = 0;
// 					}
// 					else
// 					{
// 						watchdog_errors++;
// 					}
// 					last_pv_time = pv_time;
// 				}
// 				if (watchdog_errors >= cfg_val[c_watchdog_errors])
// 				{
// 					printLog("Watchdog detected problem. Stopping");
// 					running = 2;
// 				}
// 			}
// 		}
// 		else
// 		{
// 			watchdog_errors = 0;
// 		}
// 		if (++onesec_check >= 10)
// 		{
// 			// run check on background boxing every 10 ticks and check for video timer if capturing
// 			onesec_check = 0;
// 			// 4.9 compiler seems to want a print after the box finish to get input FIFO working again
// 			if (check_box_files())
// 				printLog("Removed item from Box Queue\n");
// 			// Check to make sure image operation not stuck (no callback) if enabled
// 			if ((cfg_val[c_callback_timeout] > 0) && i_capturing)
// 			{
// 				i_capturing--;
// 				if (i_capturing == 0)
// 				{
// 					printLog("Image capture timed out %s\n", filename_image);
// 					close_img(0);
// 				}
// 			}
// 			if (v_capturing && video_stoptime > 0)
// 			{
// 				if (time(NULL) >= video_stoptime)
// 				{
// 					printLog("Stopping video from timer\n");
// 					stop_video(0);
// 					if (cfg_val[c_video_split] > 0 && (video_stoptimeEnd == 0 || video_stoptimeEnd > time(NULL)))
// 					{
// 						video_stoptime = time(NULL) + cfg_val[c_video_split];
// 						if (video_stoptimeEnd != 0 && video_stoptime >= video_stoptimeEnd)
// 						{
// 							video_stoptime = video_stoptimeEnd;
// 						}
// 						printLog("Restarting next split of %d seconds\n", cfg_val[c_video_split]);
// 						start_video(0);
// 					}
// 				}
// 			}
// 		}
// 		usleep(cfg_val[c_fifo_interval]);
// 	}

// 	close(fd);
// 	if (system("killall motion 2> /dev/null") == -1)
// 		error("Could not stop external motion", 1);

// 	printLog("SIGINT/SIGTERM received, stopping\n");
// 	//
// 	// tidy up
// 	//
// 	if (!idle)
// 		stop_all();
// 	if (running == 0)
// 		exec_macro(cfg_stru[c_startstop], "stop");
// 	else
// 		exec_macro(cfg_stru[c_startstop], "watchdog");
// 	return 0;
// }