#include"IPC.h"


using namespace jmxRCore;


void IPCMutex::lock()
{
	if (mMutex)
	{
		::WaitForSingleObject(mMutex, INFINITY);
	}

}

void IPCMutex::unlock()
{
	if (mMutex)
	{
		::ReleaseMutex(mMutex);
	}
}

void IPCMutex::destory()
{
	if (mMutex)
	{
		::CloseHandle(mMutex);
		mMutex = nullptr;
	}
}


IPCManager::IPCManager(const std::string& fileName,
	u32 memSize, bool isCreate) :name(fileName), size(memSize), buffer(nullptr)
{
	if (isCreate)
	{
		fileMapping = ::CreateFileMappingA(INVALID_HANDLE_VALUE, NULL,
			PAGE_READWRITE,
			0, size, name.c_str());
	}
	else
	{
		fileMapping = ::OpenFileMappingA(FILE_MAP_ALL_ACCESS, false, name.c_str());
	}

	Assert(fileMapping);

	buffer = static_cast<u8*>(::MapViewOfFile(fileMapping, FILE_MAP_ALL_ACCESS, 0, 0, 0));
	Assert(buffer);
	mutex = IPCManager::getMutex(name+"_mutex", isCreate);


	if (isCreate)
	{
		IPCMutexGuard guard(mutex);
		auto header = (IPCHeader*)buffer;
		header->MsgNum = 0;
		header->curPtr = sizeof(IPCHeader);
		header->leftMem = size - header->curPtr;
		header->lastPtr = header->curPtr;
	}

	create = isCreate;


}

IPCManager::~IPCManager()
{
	if (buffer) {
		::UnmapViewOfFile(buffer);
		buffer = nullptr;
	}
	if (fileMapping) {
		CloseHandle(fileMapping);
		fileMapping = nullptr;
	}
	size = 0;

	if (create)
		mutex.destory();
}

bool IPCManager::empty()
{
	IPCMutexGuard guard(mutex);
	auto s = ((IPCHeader*)(buffer))->MsgNum;
	return s == 0;
}

bool IPCManager::full()
{
	IPCMutexGuard guard(mutex);
	auto s = ((IPCHeader*)(buffer))->leftMem;

	return s > 0;
}

u32 IPCManager::getSize()
{
	IPCMutexGuard guard(mutex);
	auto s = ((IPCHeader*)(buffer))->MsgNum;

	return s;
}


bool IPCManager::pushMsg(EIPCMsg msgType,
	u32 x, u32 y, u32 z,
	u32 byteSize,
	EIPCMisc misc,
	u32 stride,
	void* data)
{
	
	IPCMessage msg;
	msg.msgType = msgType;
	msg.xSize = x;
	msg.ySize = y;
	msg.zSize = z;
	msg.byteSize = byteSize;
	msg.miscType = misc;
	msg.stride = stride;
	

	IPCMutexGuard guard(mutex);
	auto header = (IPCHeader*)buffer;
	auto needMem = sizeof(IPCMessage) + byteSize;
	if (header->leftMem < needMem)
		return false;
	
	msg.lastPtr = header->lastPtr;
	
	auto dest = buffer + header->curPtr;
	memcpy_s(dest, sizeof(msg), &msg, sizeof(msg));
	dest += sizeof(msg);
	memcpy_s(dest, byteSize, data, byteSize);


	header->MsgNum += 1;
	header->lastPtr = header->curPtr;
	header->curPtr += needMem;
	header->leftMem -= needMem;

	return true;
}


void* IPCManager::popMsg(_Para_Out IPCMessage* msg)
{
	if (empty())
		return nullptr;

	IPCMutexGuard guard(mutex);
	auto header = (IPCHeader*)buffer;

	//获取Msg
	auto pMsg = (IPCMessage*)(buffer + header->lastPtr);
	msg->msgType = pMsg->msgType;
	msg->xSize = pMsg->xSize;
	msg->ySize = pMsg->ySize;
	msg->zSize = pMsg->zSize;
	msg->byteSize = pMsg->byteSize;
	msg->miscType = pMsg->miscType;
	msg->stride = pMsg->stride;
	msg->lastPtr = pMsg->lastPtr;

	auto desc = std::malloc(msg->byteSize);

	memcpy_s(desc, msg->byteSize, buffer + header->lastPtr + sizeof(IPCMessage), msg->byteSize);

	//栈指针回退
	header->MsgNum -= 1;
	header->leftMem += sizeof(IPCMessage) + msg->byteSize;
	header->curPtr = header->lastPtr;
	header->lastPtr = msg->lastPtr;
	
	return desc;
}


IPCMutex IPCManager::getMutex(const std::string& name, bool isCreate)
{
	HANDLE _mutex = nullptr;
	if (isCreate)
	{
		_mutex = ::CreateMutexA(NULL, false, name.c_str());
	}
	else
	{
		_mutex = ::OpenMutexA(SYNCHRONIZE, false, name.c_str());
	}
	assert(_mutex);

	return IPCMutex(name, _mutex);;
}