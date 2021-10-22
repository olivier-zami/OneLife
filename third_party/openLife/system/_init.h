//
// Created by olivier on 20/10/2021.
//

#ifndef ONELIFE_SYSTEM_INIT_H
#define ONELIFE_SYSTEM_INIT_H

#if defined(__linux__)
#define SYSTEM linux

#elif defined(_WIN32)||defined(WIN32)
#define SYSTEM win32

#elif defined(__mac__)
#define SYSTEM mac

#else
#define SYSTEM system

#endif

#endif //ONELIFE_SYSTEM_INIT_H
