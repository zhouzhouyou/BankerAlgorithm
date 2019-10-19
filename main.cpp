#include <iostream>
#include <Windows.h>
#include <fstream>
#include <cstdlib>
#include <string>
#include <algorithm>

#define CUSTOMERS_NUM  5
#define RESOURCES_NUM  3
#define RUNNING_TIME_OUT 100000

using std::cout;
using std::endl;
using std::ios;
using std::ifstream;
using std::string;
using std::to_string;
using std::min;



/*available resource, update every time*/
int available_resource[RESOURCES_NUM] = { 0 };

int future_available[RESOURCES_NUM] = { 0 };

/*max resource of each customer, 
 *after the customer reach its max, 
 *it will release all resources after some time*/
int max_resource[CUSTOMERS_NUM][RESOURCES_NUM] = { 0 };

/*needed resource of each customer*/
int need[CUSTOMERS_NUM][RESOURCES_NUM] = { 0 };

int future_need[CUSTOMERS_NUM][RESOURCES_NUM] = { 0 };

/*allocation of resources*/
int allocation_resource[CUSTOMERS_NUM][RESOURCES_NUM] = { 0 };

int future_allocation[CUSTOMERS_NUM][RESOURCES_NUM] = { 0 };

/*whether the customer has been finished XD*/
bool finish[CUSTOMERS_NUM] = { false };

bool future_finish[CUSTOMERS_NUM] = { false };

int ids[CUSTOMERS_NUM] = { 0, 1, 2, 3, 4 };

int finished = 0;
int future_finished = 0;

HANDLE mutex = CreateMutex(nullptr, FALSE, nullptr);
HANDLE handleOfCustomerThread[CUSTOMERS_NUM];
DWORD  idOfCustomerThread[CUSTOMERS_NUM];

DWORD WINAPI customer(LPVOID param);

/**
 * \brief initial arrays from files
 */
void ShowState();


/**
 * \brief initial arrays from files
 */
void InitialArrays();


/**
 * \brief initial threads
 */
void InitialThreads();


/**
 * \brief customer request resource from bank
 * \param customer_number which customer request resource
 * \param request how much resource to request
 * \return 0 if success, 1 if can release, -1 if fail
 */
int Request(const int& customer_number, int request[]);

/**
 * \brief check if the system is in safe mode
 * \return true if safe
 */
bool SafeMode();

/**
 * \brief check if have enought resource for this customer in future
 * \param customer_number customer
 * \return 0 if enought in future
 */
bool FutureEnoughResource(const int& customer_number);



/**
 * \brief customer release all its resource to bank
 * \param customer_number which customer release all its resources
 * \return 0 if success
 */
int Release(const int& customer_number);

int main()
{
	InitialArrays();
	InitialThreads();
	Sleep(RUNNING_TIME_OUT);
	for (auto i = 0; i < CUSTOMERS_NUM; i++) {
		CloseHandle(handleOfCustomerThread[i]);
	}
	cout << "if not finished yet, it means that there is no solution at all";
}

DWORD __stdcall customer(LPVOID param)
{
	int customer_id = *(int*)(param);
	string thread_created = "thread " + to_string(customer_id) + " created.\n";
	cout << thread_created;
	srand(GetCurrentThreadId() * 100);
	while (true)
	{
		Sleep(rand() % 1000);
		WaitForSingleObject(mutex, INFINITE);
		string customer_get_mutex = "---customer " + to_string(customer_id) + " get     mutex---\n";
		cout << endl << customer_get_mutex;

		int request[RESOURCES_NUM];
		for (auto i = 0; i < RESOURCES_NUM; i++)
			request[i] = rand() % (min(need[customer_id][i], available_resource[i]) + 1);

		bool all_zero = true;
		for (auto i = 0; i < RESOURCES_NUM; i++) {
			if (request[i] > 0) {
				all_zero = false;
				break;
			}
		}
		if (all_zero) {
			for (auto i = 0; i < RESOURCES_NUM; i++) {
				if (need[customer_id][i] > 0) {
					request[i] = 1;
					break;
				}
			}
		}

		/*cout << "---request created---" << endl;
		for (const auto& i : request) cout << i << " ";
		cout << endl << "---requesting---" << endl;*/

		int result = Request(customer_id, request);
		switch (result)
		{
		case -1:
			cout << "fail to get resource" << endl;
			break;
		case 1:
			Release(customer_id);
			break;
		default:
			cout << "sucess to get resource" << endl;
			break;
		}
		string customer_release_mutex = "---customer " + to_string(customer_id) + " release mutex---\n";
		cout << customer_release_mutex << endl;
		ReleaseMutex(mutex);
		if (result == 1) break;
	}
	
	return 0;
}

void ShowState()
{
	cout << endl << "------max        resource------" << endl;
	for (const auto& i : max_resource)
	{
		for (const auto& j : i)
			cout << j << " ";
		cout << endl;
	}

	cout << "------available  resource------" << endl;
	for (const auto& i : available_resource)
		cout << i << " ";

	cout << endl << "------allocation resource------" << endl;
	for (const auto& i : allocation_resource)
	{
		for (const auto& j : i)
			cout << j << " ";
		cout << endl;
	}

	cout << "------need       resource------" << endl;
	for (const auto& i : need) {
		for (const auto& j : i)
			cout << j << " ";
		cout << endl;
	}
	cout << endl;
}

void InitialArrays()
{
	cout << "initializing arrays" << endl;
	ifstream arrays("arrays.txt", ios::in);
	if (!arrays)
	{
		cout << "fail to open txt file" << endl;
		return;
	}
	for (auto& i : max_resource)
		for (auto& j : i)
			arrays >> j;
	for (auto& i : available_resource)
		arrays >> i;
	for (auto& i : allocation_resource)
		for (auto& j : i)
			arrays >> j;
	for (auto i = 0; i < CUSTOMERS_NUM; i++) {
		for (auto j = 0; j < RESOURCES_NUM; j++) {
			need[i][j] = max_resource[i][j] - allocation_resource[i][j];
		}
	}

	arrays.close();

	ShowState();
}

void InitialThreads()
{
	cout << "------initializing threads------" << endl;
	for (auto i = 0; i < CUSTOMERS_NUM; i++) {
		handleOfCustomerThread[i] = CreateThread(
			NULL,
			0,
			&customer,
			&ids[i],
			0,
			&idOfCustomerThread[i]
		);
	}
}

int Request(const int& customer_number, int request[])
{
	cout << "get request from " << customer_number << endl;
	for (auto i = 0; i < RESOURCES_NUM; i++) {
		cout << request[i] << " ";
	}
	cout << endl;

	/*
		temporarily take from available resource
		temporarily add to allocation_resource
	*/
	for (auto i = 0; i < RESOURCES_NUM; i++) {
		int res = request[i];
		if (res == 0) continue;
		available_resource[i] -= res;
		allocation_resource[customer_number][i] += res;
		need[customer_number][i] -= res;
	}
	
	/*
		return to the previous status if not safe
	*/
	if (!SafeMode()) {
		for (auto i = 0; i < RESOURCES_NUM; i++) {
			int res = request[i];
			if (res == 0) continue;
			available_resource[i] += res;
			allocation_resource[customer_number][i] -= res;
			need[customer_number][i] += res;
		}
		return -1;
	}


	/*
		check if the customer can release all resource
	*/
	bool filled = true;
	for (auto i = 0; i < RESOURCES_NUM; i++) {
		if (need[customer_number][i]) {
			filled = false;
			break;
		}
	}

	if (filled) return 1;

	return 0;
}

bool SafeMode()
{
	memcpy(future_allocation, allocation_resource, CUSTOMERS_NUM * RESOURCES_NUM * sizeof(int));
	memcpy(future_need, need, CUSTOMERS_NUM * RESOURCES_NUM * sizeof(int));
	memcpy(future_available, available_resource, RESOURCES_NUM * sizeof(int));
	memcpy(future_finish, finish, CUSTOMERS_NUM * sizeof(bool));
	future_finished = finished;

	while (future_finished < CUSTOMERS_NUM) {
		bool valid = false;
		for (auto i = 0; i < CUSTOMERS_NUM; i++) {
			if (future_finish[i]) continue;
			if (FutureEnoughResource(i)) {
				future_finish[i] = true;
				future_finished++;
				for (auto j = 0; j < RESOURCES_NUM; j++) {
					future_need[i][j] = 0;
					future_available[j] += future_allocation[i][j];
					future_allocation[i][j] = 0;
				}
				valid = true;
			}
		}
		if (!valid) return future_finished == CUSTOMERS_NUM;
	}
	
	return true;
}

bool FutureEnoughResource(const int& customer_number)
{
	for (auto i = 0; i < RESOURCES_NUM; i++) {
		if (future_available[i] < future_need[customer_number][i]) return false;
	}
	return true;
}


int Release(const int& customer_number)
{
	cout << "ready to release resources from " << customer_number << endl;
	for (auto i = 0; i < RESOURCES_NUM; i++) {
		available_resource[i] += allocation_resource[customer_number][i];
		allocation_resource[customer_number][i] = 0;
	}
	finish[customer_number] = true;
	finished++;
	ShowState();
	return 0;
}
