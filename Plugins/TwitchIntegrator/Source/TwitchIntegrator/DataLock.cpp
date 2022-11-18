#include "DataLock.h"


static std::mutex ArrayMutex;
DataLock::DataLock()
{
}

DataLock::~DataLock()
{
}

std::mutex* DataLock::GetArrayMutex()
{
	return &ArrayMutex;
}
