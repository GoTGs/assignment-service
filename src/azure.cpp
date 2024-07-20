#include "../include/azure.hpp"

Blobs* Blobs::azureInstance = nullptr;
std::mutex Blobs::azureMutex;