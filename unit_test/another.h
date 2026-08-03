// 主要作用是验证多编译单元是否能正常工作

#pragma once
#ifndef ANOTHER_H_BOKUMEIDOCPP
#define ANOTHER_H_BOKUMEIDOCPP

#include<iostream>
#ifdef __GNUC__ 
#define _MEIDO_DEPRECATED(msg) __attribute__((deprecated(msg)))
#define _EXPORT __attribute__ ((visibility ("default")))
#define _HIDDEN __attribute__((visibility("hidden")))
#elif defined(_WIN32)
#define _MEIDO_DEPRECATED(msg) __declspec(deprecated(msg))
#define _EXPORT __declspec(dllexport)
#define _HIDDEN
#else
#define _MEIDO_DEPRECATED(msg)
#define _EXPORT
#define _HIDDEN
#endif 


void _EXPORT anotherTest();

#endif    // !ANOTHER_H_BOKUMEIDOCPP