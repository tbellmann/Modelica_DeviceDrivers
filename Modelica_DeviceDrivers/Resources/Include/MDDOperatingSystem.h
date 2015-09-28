/** Functions which provide an interface to some handy OS calls  (header-only library).
 *
 * @file
 * @author		Tobias Bellmann <tobias.bellmann@dlr.de>
 * @since		2014-04-13
 * @copyright Modelica License 2
 *
 * Little handy functions offered in the OperatingSystem package.
 */

#ifndef MDDOPERATINGSYSTEM_H_
#define MDDOPERATINGSYSTEM_H_

#include <stdlib.h>

#include <time.h>
#include "../src/include/CompatibilityDefs.h"
#include "ModelicaUtilities.h"

DllExport double MDD_OS_getRandomNumberDouble(double minValue, double maxValue) {
  static int _randomGeneratorInitialized = 0;
  int randomInteger;
  double randomDouble;
  if(!_randomGeneratorInitialized)
  {
    srand ( clock() * time(NULL) );
    _randomGeneratorInitialized = 1;
  }
  randomInteger = rand();
  randomDouble = ((double)randomInteger/(double)RAND_MAX) * (maxValue - minValue) + minValue;
  return randomDouble;
}

#if defined(_MSC_VER)
#include <winsock2.h>
DllExport void MDD_OS_Sleep(double sleepingTime)
  {

    int time_ms = (int)(sleepingTime*1000);
    Sleep(time_ms);

  }
#else
  #include <unistd.h>
  void MDD_OS_Sleep(double sleepingTime)
  {
    sleep((int)sleepingTime);
  }
#endif /* _MSC_VER */

#endif /* MDDOPERATINGSYSTEM_H_ */
