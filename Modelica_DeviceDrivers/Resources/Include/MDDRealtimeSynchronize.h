/** Synchronize execution with real-time (header-only library).
 *
 * @file
 * @author		Tobias Bellmann <tobias.bellmann@dlr.de> (Windows)
 * @author		Bernhard Thiele <bernhard.thiele@dlr.de> (Linux)
 * @version	$Id: MDDRealtimeSynchronize.h 15720 2012-06-05 21:32:39Z thie_be $
 * @since		2012-05-29
 * @copyright Modelica License 2
 */

#ifndef MDDREALTIMESYNCHRONIZE_H_
#define MDDREALTIMESYNCHRONIZE_H_
double MDD_lastTimestamp = 0;
#include "ModelicaUtilities.h"

#if defined(_MSC_VER)

  #include <winsock2.h>
  #include "../src/include/CompatibilityDefs.h"
  #pragma comment( lib, "Winmm.lib" )

DllExport void MDD_setPriority(int priority)
  {
    ModelicaFormatMessage("ProcessPriority: %d!!\n",priority);
    switch(priority)
    {
      case -2:
        ModelicaFormatMessage("setting...\n");
        if(SetPriorityClass(GetCurrentProcess(),IDLE_PRIORITY_CLASS))
          ModelicaFormatMessage("ProcessPriority set to idle.\n");
      break;

      case -1:
        ModelicaFormatMessage("setting...\n");
        if(SetPriorityClass(GetCurrentProcess(),BELOW_NORMAL_PRIORITY_CLASS))
          ModelicaFormatMessage("ProcessPriority set to below normal.\n");
      break;

      case 0:
        ModelicaFormatMessage("setting...\n");
        if(SetPriorityClass(GetCurrentProcess(),NORMAL_PRIORITY_CLASS))
          ModelicaFormatMessage("ProcessPriority set to normal.\n");
      break;

      case 1:
        ModelicaFormatMessage("setting...\n");
        if(SetPriorityClass(GetCurrentProcess(),HIGH_PRIORITY_CLASS))
          ModelicaFormatMessage("ProcessPriority set to high.\n");
      break;

      case 2:
        ModelicaFormatMessage("setting...\n");
        if(SetPriorityClass(GetCurrentProcess(),REALTIME_PRIORITY_CLASS))
          ModelicaFormatMessage("ProcessPriority set to realtime.\n");
      break;

      default:
        ModelicaFormatMessage("setting...\n");
        if(SetPriorityClass(GetCurrentProcess(),NORMAL_PRIORITY_CLASS))
          ModelicaFormatMessage("ProcessPriority set to normal.\n");
      break;
    }

    ModelicaFormatMessage("LastError: %d\n",GetLastError());
  }

  /** Request time from a monotonic increasing real-time clock.
   *
   * @param[in] resolution windows specific clock resolution (ignored in linux).
   * @return (ms) time in milliseconds.
   */
 double MDD_getTimeMS(int resolution)
  {

	  LARGE_INTEGER ticks, frequency;

	  QueryPerformanceFrequency(&frequency); 
	  QueryPerformanceCounter(&ticks);
	  ticks.QuadPart *= 1000000;
	  ticks.QuadPart /= (frequency.QuadPart);
	  
	  return (double)ticks.QuadPart / 1000.0;

  };
 double   MDD_timestamp()
	 //returns a timestamp with 1ms accuracy (Vista or higher)
 {

	 SYSTEMTIME systemTime;
	 double timestamp;
	 // get the system time and the qpc;
	 GetSystemTime(&systemTime);
	 // calculate the base timestamp from system time
	 timestamp = (double)systemTime.wHour * 3600  + (double)systemTime.wMinute * 60  + (double)systemTime.wSecond + (double)systemTime.wMilliseconds / 1000;
	 return timestamp;
 }

  long long MDD_QPCMicroseconds()
  {
	  LARGE_INTEGER ticks, frequency;

	  QueryPerformanceFrequency(&frequency); 
	  QueryPerformanceCounter(&ticks);
	  ticks.QuadPart *= 1000000;
	  ticks.QuadPart /= frequency.QuadPart;
	  return ticks.QuadPart;

  }
  
  double MDD_realtimeSynchronize(double simTime, int resolution, double * integratorTimeStep)
  {
    static long long MDD_lastTime = 0;
    static long long MDD_startTime = 0;
    static double MDD_lastSimTime = 0;
    static double MDD_lastintegratorTimeStep = 0;
	double calculationTime = 0;
	long long simTimeMicroseconds;
	long long timeLeft;
	int sleepCycleMs;
	//int debugCounter=0;
	//int debugCounter2=0;
	if (MDD_startTime == 0) MDD_startTime =  MDD_QPCMicroseconds();
	if (MDD_lastTime == 0) MDD_lastTime =  MDD_QPCMicroseconds();
	
	*integratorTimeStep = 0;
	
	
	if(simTime != MDD_lastSimTime)
    {
		calculationTime = ((double)(MDD_QPCMicroseconds() - MDD_lastTime))/1000000.0;
		*integratorTimeStep = simTime - MDD_lastSimTime;
		
		simTimeMicroseconds = simTime * 1000 * 1000;
		timeLeft = simTimeMicroseconds - (MDD_QPCMicroseconds() - MDD_startTime);
		
		while(timeLeft > 0)
		{
			
			if (timeLeft > 1000)
			{
				sleepCycleMs = timeLeft / 1000;
				if(timeBeginPeriod(1))
					ModelicaFormatError("realtimeSynchronize: Resolution not supported by hardware!");
				Sleep(sleepCycleMs);
				timeEndPeriod(1);
				//debugCounter2+=sleepCycleMs;
			}
			else
			{
				Sleep(0);
				//debugCounter++;
			}

			 
			 timeLeft = simTimeMicroseconds - (MDD_QPCMicroseconds() - MDD_startTime);
		}
		//*integratorTimeStep = debugCounter;
		MDD_lastSimTime = simTime;
		MDD_lastTime = MDD_QPCMicroseconds();
		
		MDD_lastintegratorTimeStep = *integratorTimeStep;
	}
	return calculationTime;
	//return debugCounter2;
	
	
  }
#elif defined(__linux__)

  #include <time.h>
  #include <sched.h>
  #include <math.h>
  #include <stdlib.h>
  #include <unistd.h>
  #include <errno.h>
  #include <sys/mman.h>
  #include "../src/include/CompatibilityDefs.h"
  #define MY_RT_PRIORITY (49) /**< we use 49 since PRREMPT_RT use 50
                                 as the priority of kernel tasklets
                                 and interrupt handlers by default */
  #define NSEC_PER_SEC    (1000000000) /* The number of nsecs per sec. */

  /** Set process priority.
   *
   * Function maps directly on windows API. For linux a mapping was chosen that seemed
   * seemed to be sensible.
   * @param[in] priority range: (-2: idle, -1: below normal, 0: normal, 1: high, 2: realtime)
   */
  void MDD_setPriority(int priority) {
    int ret;
    struct sched_param param;
    errno = 0; /* zero out errno since -1 may be a valid return value for nice(..) and not necessarily indicate error */

    ModelicaFormatMessage("Trying to set ProcessPriority: %d.\n",priority);

    switch(priority) {
      case -2:
        ret = nice(20);
        if (ret == -1 && errno != 0) {
          ModelicaError("MDDRealtimeSynchronize.h: nice(20) failed\n");
          exit(-1);
        } else {
          ModelicaFormatMessage("ProcessPriority set to %d=nice(20) \"idle\".\n", ret);
        }
      break;

      case -1:
        ret = nice(10);
        if (ret == -1 && errno != 0) {
          ModelicaError("MDDRealtimeSynchronize.h: nice(10) failed\n");
          exit(-1);
        } else {
          ModelicaFormatMessage("ProcessPriority set to %d=nice(10) \"below normal\".\n", ret);
        }
      break;

      case 0:
        ret = nice(0);
        if (ret == -1 && errno != 0) {
          ModelicaError("MDDRealtimeSynchronize.h: nice(0) failed\n");
          exit(-1);
        } else {
          ModelicaFormatMessage("ProcessPriority set to %d=nice(0) \"normal\".\n", ret);
        }
      break;

      case 1:
        ModelicaFormatMessage("ProcessPriority \"high\" needs generally *root* privileges! Trying..\n");
        ret = nice(-20);
        if (ret == -1 && errno != 0) {
          ModelicaError("MDDRealtimeSynchronize.h: nice(-20) failed\n");
          exit(-1);
        } else {
          ModelicaFormatMessage("ProcessPriority set to %d=nice(-20) \"high\".\n", ret);
        }
      break;

      case 2:
        ModelicaFormatMessage("ProcessPriority \"Realtime\" needs generally *root* privileges and a real-time kernel (PRREMPT_RT) for hard realtime! Trying..\n");

         /* Lock entire address space into physical memory */
        if(mlockall(MCL_CURRENT | MCL_FUTURE) == -1) {
          ModelicaError("MDDRealtimeSynchronize.h: mlockall failed\n");
          exit(-1);
        }

        /* Declare ourself as a real time task */
        param.sched_priority = MY_RT_PRIORITY;
        if(sched_setscheduler(0, SCHED_FIFO, &param) == -1) {
          ModelicaError("MDDRealtimeSynchronize.h: sched_setscheduler failed\n");
          exit(-1);
        } else {
          ModelicaFormatMessage("ProcessPriority set to \"Realtime\"!\n");
        }
      break;

      default:
        ModelicaFormatMessage("Using default process priority\n");
      break;
    }

  }

  /** Slow down task so that simulation time == real-time.
   *
   * @param[in] simTime (s) simulation time
   * @param[in] resolution windows specific clock resolution. Ignored in linux.
   * @param[out] availableTime time that is left before realtime deadline is reached.
   * @return (s) Time between invocation of this function, i.e. "computing time" in seconds
   */
  double MDD_realtimeSynchronize(double simTime, int resolution, double * availableTime) {
    static int initialized = 0;
    static struct timespec t_start = { .tv_sec = 0, .tv_nsec = 0 };
    static struct timespec t_clockRealtime = { .tv_sec = 0, .tv_nsec = 0 }; /* current/last time of real time clock */
    struct timespec t_abs; /* Absolute time until which execution will be delayed (to catch up with real-time) */
    double fractpart, intpart, deltaTime;
    int ret;

    if (!initialized) {
      ret = clock_gettime(CLOCK_MONOTONIC, &t_start);
      if (ret) {
	ModelicaError("MDDRealtimeSynchronize.h: clock_gettime(..) failed\n");
	exit(-1);

      }
      t_clockRealtime = t_start;
      initialized = 1;
    }

   /* save away value of last time that the real-time clock was inquired */
   deltaTime = t_clockRealtime.tv_sec + (double)t_clockRealtime.tv_nsec/NSEC_PER_SEC;
   /* get value the current time of the real-time clock */
   clock_gettime(CLOCK_MONOTONIC, &t_clockRealtime);
   /* Calculate the deltaTime (=calculation time) by subtracting the
    * old value of the real-time clock from the current (new) value of the real-time clock */
   deltaTime = (t_clockRealtime.tv_sec + (double)t_clockRealtime.tv_nsec/NSEC_PER_SEC) - deltaTime;

   /* convert simulation time to corresponding real-time clock value */
   fractpart = modf(simTime, &intpart);
   t_abs.tv_sec = t_start.tv_sec + (time_t) intpart;
   t_abs.tv_nsec = t_start.tv_nsec + (long) floor(fractpart*NSEC_PER_SEC);
   while (t_abs.tv_nsec >= NSEC_PER_SEC) {
     t_abs.tv_nsec -= NSEC_PER_SEC;
     t_abs.tv_sec++;
   }


   *availableTime = ( t_abs.tv_sec - t_clockRealtime.tv_sec )
      + ((double)t_abs.tv_nsec - (double)t_clockRealtime.tv_nsec)/NSEC_PER_SEC;
   /* printf("t_abs.tv_sec: %d, t_cr.tv_sec: %d, t_abs.tv_nsec: %d, t_cr.tv_nsec: %d\n", t_abs.tv_sec, t_clockRealtime.tv_sec, t_abs.tv_nsec, t_clockRealtime.tv_nsec); */

   /* wait until simulation time == real-time */
   ret = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &t_abs, NULL);
   if (ret) {
      ModelicaError("MDDRealtimeSynchronize.h: clock_nanosleep(..) failed\n");
      exit(1);
   }

   /* get value the current time of the real-time clock (should be equal to t_abs if everything is OK) */
   clock_gettime(CLOCK_MONOTONIC, &t_clockRealtime);

   return deltaTime;
  }


  double MDD_getTimeMS(int resolution) {
    /* argument resolution is ignored */
    struct timespec ts;
    int ret, ms;

    ret = clock_gettime(CLOCK_MONOTONIC, &ts);
    if (ret) {
        ModelicaFormatError("MDDRealtimeSynchronize.h: clock_gettime failed (%s)\n", strerror(errno));
        exit(-1);
    }

    ms = ts.tv_sec*1000 + floor( (double)ts.tv_nsec/1000.0 + 0.5);

    return (double)ms;

  };

double MDD_timestamp()
{
	struct timeval tv;
 struct tm *ptm;

 double milliseconds;
double timestamp;
 /* Obtain the time of day, and convert it to a tm struct. */
 gettimeofday (&tv, NULL);
 ptm = gmtime (&tv.tv_sec);


 /* Compute second fraction from microseconds. */
 milliseconds = (double)tv.tv_usec / 1000000.0;
 timestamp = ptm->tm_hour * 3600 + ptm->tm_min * 60 + ptm->tm_sec + milliseconds;
 return timestamp;
}

#else

  #error "Modelica_DeviceDrivers: No support of real-time synchronization for your platform"

#endif /* defined(_MSC_VER) */

double MDD_measureTime(double dummy,const char * message)
{
	double deltaT;
	if (MDD_lastTimestamp == 0)
		MDD_lastTimestamp = MDD_getTimeMS(1);
	
	deltaT = MDD_getTimeMS(1) - MDD_lastTimestamp;
	ModelicaFormatMessage("%s, DeltaT: %f\n", message, deltaT);
	MDD_lastTimestamp = MDD_getTimeMS(1);
	return deltaT;
}
#endif /* MDDREALTIMESYNCHRONIZE_H_ */
