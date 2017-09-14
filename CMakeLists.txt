#
#	Makefile for 42minjoy
#
cmake_minimum_required(VERSION 3.0)
project(42minjoy VERSION 1.0)
set(CMAKE_BUILD_TYPE Release)
add_definitions(-D_CRT_SECURE_NO_WARNINGS)
add_executable(42minjoy joy.c)
