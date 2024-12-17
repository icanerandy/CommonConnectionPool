#include "ConnectionPool.h"

// 线程安全的懒汉单例函数接口
ConnectionPool* ConnectionPool::getConnectionPool()
{
	static ConnectionPool pool;	// 静态局部变量的初始化，由编译器自动实现lock和unlock
	return &pool;
}

// 从配置文件中加载配置项
bool ConnectionPool::loadConfigFile()
{
	FILE* pf = fopen("mysql.ini", "r");
	if (pf == nullptr)
	{
		LOG("mysql.ini file does not exist!");
		return false;
	}

	while (!feof(pf))
	{
		char line[1024] = { 0 };
		fgets(line, 1024, pf);
		std::string str = line;

		int idx = str.find('=');
		if (idx == -1)	// 无效的配置项/注释
		{
			continue;
		}

		// username=root\n
		int endIdx = str.find('\n', idx);
		std::string key = str.substr(0, idx);
		std::string value = str.substr(idx + 1, endIdx - idx - 1);

		if (key == "ip")
		{
			_ip = value;
		}
		else if (key == "port")
		{
			_port = atoi(value.c_str());
		}
		else if (key == "username")
		{
			_username = value;
		}
		else if (key == "password")
		{
			_password = value;
		}
		else if (key == "dbname")
		{
			_dbname = value;
		}
		else if (key == "initSize")
		{
			_initSize = atoi(value.c_str());
		}
		else if (key == "maxSize")
		{
			_maxSize = atoi(value.c_str());
		}
		else if (key == "maxIdleTimeout")
		{
			_maxIdleTime = atoi(value.c_str());
		}
		else if (key == "connectionTimeout")
		{
			_connectionTimeout = atoi(value.c_str());
		}
	}

	return true;
}

// 运行在独立线程中，专门负责生产新的连接
void ConnectionPool::produceConnectionTask()
{
	for (;;)
	{
		std::unique_lock<std::mutex> lock(_queueMutex);
		while (!_connectionQueue.empty())
		{
			_cv.wait(lock);	// 队列不空，此处生产线程进入等待状态
		}

		// 连接数量没有到达上限，继续创建新的连接
		if (_connectionCnt < _maxSize)
		{
			Connection* conn = new Connection();
			conn->connect(_ip, _port, _username, _password, _dbname);
			_connectionQueue.push(conn);
			_connectionCnt++;
		}

		// 通知消费者线程可以消费连接了（消费者由等待 => 阻塞）
		_cv.notify_all();
	}
}

// 连接池的构造
ConnectionPool::ConnectionPool()
{
	// 加载配置项
	if (!loadConfigFile())
	{
		return;
	}

	// 创建初始数量的连接
	for (size_t i = 0; i < _initSize; ++i)
	{
		Connection* conn = new Connection();
		conn->connect(_ip, _port, _username, _password, _dbname);
		_connectionQueue.push(conn);
		_connectionCnt++;
	}

	// 启动一个新的线程，作为连接的生产者 Linux下thread底层调用的就是pthread
	std::thread produceTh(std::bind(&ConnectionPool::produceConnectionTask, this));
}
