#include"../Config/JmxRConfig.h"

#include<windows.h>
#include<string>
#include<queue>
namespace jmxRCore
{
	class IPCMutex
	{
	public:
		IPCMutex(){ mMutex = nullptr; }
		IPCMutex(const std::string& name, HANDLE _mutex) :
			mName(name), mMutex(_mutex){}
		~IPCMutex(){ mMutex = nullptr; }

		void lock();
		void unlock();
		void destory();


	private:
		std::string	 mName;
		HANDLE		 mMutex;

	};

	class IPCMutexGuard
	{
	public:
		IPCMutexGuard(IPCMutex& _mutex) :ipcmutex(_mutex)
		{
			ipcmutex.lock();
		}
		~IPCMutexGuard()
		{
			ipcmutex.unlock();
		}


	private:
		IPCMutex&  ipcmutex;
	};


	enum EIPCMsg
	{
		EMsg_1D_Text = 1 ,
		EMsg_1D_F32 ,
		EMsg_1D_Vec2,
		EMsg_1D_Vec3,
		EMsg_1D_U32 ,
		EMsg_2D_F32 ,
		EMsg_2D_Vec2,
		EMsg_2D_Vec3,
		EMsg_2D_U32 ,
		EMsg_3D_F32	,
		EMsg_3D_U32 ,
		EMsg_Raw ,
		EMsg_Null=0
	};

	enum EIPCMisc
	{
		EMisc_R8G8B8A8 = 1 < 1,
		EMisc_A8R8G8B8 = 1 < 2,
		EMisc_StructBuffer = 1 < 3,
		EMisc_Null=0
	};

	struct IPCMessage
	{
		EIPCMsg		msgType;
		u32			xSize;
		u32			ySize;
		u32			zSize;
		u32			byteSize;
		EIPCMisc	miscType;
		u32			stride;
		u32			lastPtr;			//消息指针，指向上一条消息的位置
	};

	struct IPCHeader
	{
		u32			MsgNum;
		u32			curPtr;
		u32			lastPtr;
		s32			leftMem;
	};

	class IPCManager
	{
	public:
		IPCManager(const std::string& fileName, u32 memSize, bool isCreate);
		~IPCManager();


		bool	empty();
		bool	full();
		u32			getSize();


		bool	pushMsg(EIPCMsg msgType, u32 x, u32 y, u32 z, u32 byteSize,
			EIPCMisc misc, u32 stride,void* data);

		void*	popMsg(_Para_Out IPCMessage* msg);
		

		
		static IPCMutex getMutex(const std::string& name, bool isCreate);

	private:
		std::string				name;
		u32						size;
		HANDLE					fileMapping;
		u8*						buffer;
		IPCMutex				mutex;
		bool					create;

	};


}