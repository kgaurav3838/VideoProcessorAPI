// PThreads_main0.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

/*
PThreads- Multithread creation sample code
*/

#include <iostream>
#include <stdlib.h>
#include <pthread.h>
#include <windows.h>
#define NUM_THREADS 3
using namespace std;

void* first_th(void* arg)
{
	int* p = (int*)arg;
	int q = *p;
	cout << "Hello 1st Thread and thread ID is: " << q << endl;
	return 0;
}
void* second_th(void* arg)
{
	int* p = (int*)arg;
	int q = *p;
	cout << "Hello 2nd Thread and thread ID is: " << q << endl;
	return 0;
}
void* third_th(void* arg)
{
	int* p = (int*)arg;
	int q = *p;
	cout << "Hello 3rd Thread and thread ID is: " << q << endl;
	return 0;
}

int main(void)
{
	int i = 0;
	pthread_t tid[NUM_THREADS];
	pthread_create(&tid[0], NULL, first_th, &(++i));
	pthread_create(&tid[1], NULL, second_th, &(++i));
	pthread_create(&tid[2], NULL, third_th, &(++i));

	pthread_join(tid[0], NULL);
	pthread_join(tid[1], NULL);
	pthread_join(tid[2], NULL);

	return 0;
}