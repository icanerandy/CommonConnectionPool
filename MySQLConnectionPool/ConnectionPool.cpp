#include "ConnectionPool.h"

// 线程安全的懒汉单例函数接口
ConnectionPool* ConnectionPool::getConnectionPool()
{
	static ConnectionPool pool;	// 静态局部变量的初始化，由编译器自动实现lock和unlock
	return &pool;
}

// 给外部提供接口，从连接池中获取一个可用的空闲连接
std::shared_ptr<Connection> ConnectionPool::getConnection()
{
	std::unique_lock<std::mutex> lock(_queueMutex);
	while (_connectionQueue.empty())
	{
		if (_cv.wait_for(lock, std::chrono::milliseconds(_connectionTimeout)) == std::cv_status::timeout)
		{
			LOG("获取空闲连接超时，获取连接失败！");
			return nullptr;
		}
	}

	/*
	shared_ptr智能指针析构时，会将connection队列资源直接delete掉，相当于调用connection的析构函数，connection就被close掉了。
	所以这里需要自定义shared_ptr的释放资源方式，把connection直接归还到queue当中
	*/
	std::shared_ptr<Connection> sp(_connectionQueue.front(),
									[&] (Connection* conn) {
										std::unique_lock<std::mutex> lock(_queueMutex);	// 这里是在服务器应用线程中调用的，所以一定要考虑队列的线程安全操作
										conn->refreshAliveTime();	// 刷新空闲时间起始点
										_connectionQueue.push(conn);
									});
	_connectionQueue.pop();
	if (_connectionQueue.empty())
	{
		_cv.notify_all();	// #1 谁消费了队列中的最后一个connection，谁就负责通知生产者生产连接
	}
	//_cv.notify_all();	// #2 消费完连接后，通知生产者线程检查一下，如果队列为空了，赶紧生产新的连接
	return sp;
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

void ConnectionPool::scannerConnectionTask()
{
	for (;;)
	{
		// 通过sleep模拟定时效果
		std::this_thread::sleep_for(std::chrono::seconds(_maxIdleTime));

		// 扫描整个队列，释放多余的连接
		std::unique_lock<std::mutex> lock(_queueMutex);
		while (_connectionCnt > _initSize)
		{
			Connection* conn = _connectionQueue.front();
			if (conn->getAliveTime() >= _maxIdleTime * 1000)
			{
				_connectionQueue.pop();
				_connectionCnt--;
				delete conn;	// 调用~Connection()释放连接
			}
			else
			{
				break;	// 队头的连接没有超过_maxIdleTime，其它连接肯定也没有（队列是队头先进）
			}
		}
	}
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
			conn->refreshAliveTime();	// 刷新空闲时间起始点
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
		conn->refreshAliveTime();	// 刷新空闲时间起始点
		_connectionQueue.push(conn);
		_connectionCnt++;
	}

	// 启动一个新的线程，作为连接的生产者 Linux下thread底层调用的就是pthread
	std::thread produceTh(std::bind(&ConnectionPool::produceConnectionTask, this));
	produceTh.detach();	// 设置为守护线程（分离）
	// 启动一个新的定时线程，扫描超过maxIdleTime时间的空闲连接，进行对多余连接的回收
	std::thread scannerTh(std::bind(&ConnectionPool::scannerConnectionTask, this));
	scannerTh.detach();	// 设置为守护线程（分离）
}
