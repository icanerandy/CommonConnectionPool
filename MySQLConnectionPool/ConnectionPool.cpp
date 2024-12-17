#include "ConnectionPool.h"

// �̰߳�ȫ���������������ӿ�
ConnectionPool* ConnectionPool::getConnectionPool()
{
	static ConnectionPool pool;	// ��̬�ֲ������ĳ�ʼ�����ɱ������Զ�ʵ��lock��unlock
	return &pool;
}

// �������ļ��м���������
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
		if (idx == -1)	// ��Ч��������/ע��
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

// �����ڶ����߳��У�ר�Ÿ��������µ�����
void ConnectionPool::produceConnectionTask()
{
	for (;;)
	{
		std::unique_lock<std::mutex> lock(_queueMutex);
		while (!_connectionQueue.empty())
		{
			_cv.wait(lock);	// ���в��գ��˴������߳̽���ȴ�״̬
		}

		// ��������û�е������ޣ����������µ�����
		if (_connectionCnt < _maxSize)
		{
			Connection* conn = new Connection();
			conn->connect(_ip, _port, _username, _password, _dbname);
			_connectionQueue.push(conn);
			_connectionCnt++;
		}

		// ֪ͨ�������߳̿������������ˣ��������ɵȴ� => ������
		_cv.notify_all();
	}
}

// ���ӳصĹ���
ConnectionPool::ConnectionPool()
{
	// ����������
	if (!loadConfigFile())
	{
		return;
	}

	// ������ʼ����������
	for (size_t i = 0; i < _initSize; ++i)
	{
		Connection* conn = new Connection();
		conn->connect(_ip, _port, _username, _password, _dbname);
		_connectionQueue.push(conn);
		_connectionCnt++;
	}

	// ����һ���µ��̣߳���Ϊ���ӵ������� Linux��thread�ײ���õľ���pthread
	std::thread produceTh(std::bind(&ConnectionPool::produceConnectionTask, this));
}
