#pragma once

#include <mutex>

/**
 * 
 */
class DataLock
{
public:
	DataLock();
	~DataLock();
	

	 static std::mutex* GetArrayMutex();
private:
};
