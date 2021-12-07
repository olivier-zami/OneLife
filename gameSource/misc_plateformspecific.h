//
// Created by olivier on 07/12/2021.
//

#ifndef INC_2HOL_MISC_PLATEFORMSPECIFIC_H
#define INC_2HOL_MISC_PLATEFORMSPECIFIC_H

// runs the platform-specific steamGateClient as a new process
// does NOT exit.
// steamGateClient runs in parallel.
// returns false if running steamGateClient is not supported on this platform
char runSteamGateClient();

// depending on directory structure for source builds, Linux app name
// might be different than Windows or Mac app name (no .exe or .app extension
// and can't be the same name as a directory).
// Example:  CastleDoctrineApp
const char *getLinuxAppName();

#endif //INC_2HOL_MISC_PLATEFORMSPECIFIC_H
