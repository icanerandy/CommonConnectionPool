#pragma once
#include <iostream>
#include <string>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <functional>
#include "Connection.h"
#include "public.h"
/*
实现连接池功能模块
*/
class ConnectionPool
{
public:
	// 获取连接池对象实例
	static ConnectionPool* getConnectionPool();

	// 给外部提供接口，从连接池中获取一个可用的空闲连接
	std::shared_ptr<Connection> getConnection();

	// 运行在独立线程中，专门负责生产新的连接
	void produceConnectionTask();

private:
	// 单例#1 构造函数私有化
	ConnectionPool();

	// 从配置文件中加载配置项
	bool loadConfigFile();

	// 扫描超过maxIdleTime时间的空闲连接，进行对多余连接的回收
	void scannerConnectionTask();

private:
	std::string _ip;			// mysql的ip地址
	unsigned short _port;		// mysql的端口号 3306
	std::string _username;		// mysql登录用户名
	std::string _password;		// mysql登录密码
	std::string _dbname;		// 连接的数据库名称
	size_t _initSize;			// 连接池的初始连接量
	size_t _maxSize;			// 连接池的最大连接量
	size_t _maxIdleTime;		// 连接池最大空闲时间
	size_t _connectionTimeout;	// 连接池获取连接的超时时间

	std::queue<Connection*> _connectionQueue;	// 存储mysql连接的队列
	std::mutex _queueMutex;		// 维护mysql连接队列的线程安全互斥锁

	atomic_int _connectionCnt;	// 记录连接所创建的connection的总数量
	std::condition_variable _cv;// 设置条件变量，用于连接生产者和消费者线程的通信
};