#include "SqlPool.h"

namespace HSLL
{
    MYSQL *SqlPool::WaitConnect()
    {
        MYSQL *pMysql;
        pool.wait_dequeue(pMysql);
        return pMysql;
    }

    void SqlPool::FreeConnect(MYSQL *pMysql)
    {
        pool.enqueue(pMysql);
    }

    bool SqlPool::Init(const char *url, const char *userName, const char *password,
                       const char *databaseName, unsigned short port, unsigned short connectMax)
    {
        for (int i = 0; i < connectMax; i++) // 建立多个连接
        {
            MYSQL *pMysql = nullptr;
            pMysql = mysql_init(pMysql);
            FALSE_LOG_RETURN(pMysql, pLog, "mysql init error");
            mysql_real_connect(pMysql, url, userName, password, databaseName, port, NULL, 0);
            FALSE_LOG_RETURN_FUNC(pMysql,mysql_close(pMysql), pLog, "mysql real connect error");
            pool.enqueue(pMysql);
        }
        return true;
    }

    void SqlPool::StopWait(unsigned int maxWaiter)
    {
        for (int i = 0; i < maxWaiter; i++)
            pool.enqueue(nullptr);
    }

    void SqlPool::Release()
    {
        MYSQL *pMysql;
        while (pool.try_dequeue(pMysql))
        {
            if (pMysql)
            mysql_close(pMysql);
        }
    }

    SqlPool::~SqlPool()
    {
        Release();
    }

    SqlPool::SqlPool(Log *pLog) : pLog(pLog) {};
}