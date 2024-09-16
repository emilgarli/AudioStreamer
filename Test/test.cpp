#include<iostream>

int main()
{
	int* arr = new int[10];
	arr[14] = 54; // crash
	// ^ this should be detected by the address sanitizer
	std::cout << arr[14] << "\n";
}