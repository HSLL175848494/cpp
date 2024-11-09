#ifndef HSLL_SQLPOOL
#define HSLL_SQLPOOL

#include <mysql/mysql.h>
#include "../Log/Log.h"
#include "../ConcurrentQueue/blockingconcurrentqueue.h"

namespace HSLL
{
    class SqlPool
    {
    private:
        Log *pLog;

        unsigned short connectMax;

        moodycamel::BlockingConcurrentQueue<MYSQL *> pool;

    public:
        ~SqlPool();

        SqlPool(Log *pLog = nullptr);

        MYSQL *WaitConnect();

        void Release();

        void StopWait(unsigned int maxWaiter);

        void FreeConnect(MYSQL *pMysql);

        bool Init(const char *url, const char *userName, const char *password,
                  const char *databaseName, unsigned short port, unsigned short connectMax = 8);
    };

}
#endif