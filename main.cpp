/*
Sleeping Barber Problem
Author: Fanyu Wang

Compilable on a standard Linux installation:
g++ -std=c++11 main.cpp -pthread
*/

#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
using namespace std;

class BarberShop
{
private:
	vector<int> waitingID;		//the IDes of customers in the waiting room
	mutex mu_semaphore, mu_waitingID, mu_print, mu_customer;		//use 4 mutexes to avoid competitive of common resourses
	condition_variable cond_barber, cond_cust;		//use 2 condition variables to wake up barber and customers
	bool semaphore = true;		//control access to the barber
	int seats;		//the number of seats in the waiting room
	int curr_cust;		//the ID of the customer who is cutting hair by the barber
	void Print();
public:
	BarberShop(int val) : seats(val) {}
	void Barber();
	void Customer(int customerID);
};

void BarberShop::Barber()		//implement of the barber
{
	while (true)
	{
		unique_lock<mutex> locker_semaphore(mu_semaphore);
		unique_lock<mutex> locker_print(mu_print);
		if (waitingID.empty())		//barber sleep if waiting room is empty
		{
			semaphore = true;		//the barber is available
			cout << "Barber sleeping" << endl;
			Print();
			locker_print.unlock();
			cond_barber.wait(locker_semaphore);		//waiting for a customer to wake up
			locker_print.lock();
			cout << "Barber wake up and cutting the hair of customer " << curr_cust << endl;
			Print();
			locker_print.unlock();
			semaphore = false;		//the barber is busy
		}
		else		//if waiting room is not empty
		{
			unique_lock<mutex> locker_waitingID(mu_waitingID);
			curr_cust = waitingID.front();		//get the ID of the first customer
			waitingID.erase(waitingID.begin());
			locker_waitingID.unlock();
			cond_cust.notify_all();		//inform the first customer to cut hair
			cout << "Barber cutting the hair of customer " << curr_cust << endl;
			Print();
			locker_print.unlock();
		}
		this_thread::sleep_for(chrono::seconds(rand() % 5 + 1));		//haircut time between 1 and 5 randomly
		locker_semaphore.unlock();
	}
}

void BarberShop::Customer(int customerID)		//implement of the customer
{
	if (waitingID.size() < seats)		//if the waiting room is not full
	{
		if (semaphore == false)		//if the barber is busy
		{
			unique_lock<mutex> locker_waitingID(mu_waitingID);
			unique_lock<mutex> locker_print(mu_print);
			waitingID.push_back(customerID);		//stand behind the queue
			cout << "Barber cutting the hair of customer " << curr_cust << endl;
			Print();
			locker_print.unlock();
			locker_waitingID.unlock();
			unique_lock<mutex> locker_customer(mu_customer);
			//wait until the barber is available and the customer is on the front of the queue
			cond_cust.wait(locker_customer, [&]() { return customerID == curr_cust; });
			locker_customer.unlock();
		}
		else		//if the barber is available
		{
			unique_lock<mutex> locker_print(mu_print);
			curr_cust = customerID;
			locker_print.unlock();
			cond_barber.notify_one();		//wake the barber up
		}
	}
	else		//leave if no seats available
		cout << "Customer " << customerID << " leaving" << endl;
}

void BarberShop::Print()		//print the waiting room list
{
	vector<int>::iterator it;
	cout << "Waiting room: ";
	for (it = waitingID.begin(); it != waitingID.end(); it++)
		cout << *it << " ";
	cout << endl;
}

void CreateCustomers(BarberShop* barberShop)		//create customers every 3 seconds
{
	int customerID = 0;
	while (true)
	{
		this_thread::sleep_for(chrono::seconds(3));
		++customerID;
		thread customer(&BarberShop::Customer, barberShop, customerID);
		customer.detach();
	}
}

int main()
{
	cout << "Please input the number of seats: ";
	int seats = 0;
	cin >> seats;

	BarberShop* barberShop = new BarberShop(seats);		//initialize a barbershop
	thread barber(&BarberShop::Barber, barberShop);		//the barber thread
	barber.detach();
	thread createCustomers(CreateCustomers, barberShop);		//a thread to create new customers
	createCustomers.join();

	return 0;
}